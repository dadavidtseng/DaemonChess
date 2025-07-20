//----------------------------------------------------------------------------------------------------
// Match.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Match.hpp"

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Network/NetworkSubsystem.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Definition/PieceDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/MatchCommon.hpp"
#include "Game/Framework/PlayerController.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Piece.hpp"
#include "Game/Subsystem/Light/LightSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
Match::Match()
{
    // Register network protocol event handlers
    RegisterNetworkCommands();

    g_theEventSystem->SubscribeEventCallbackFunction("ChessMove", OnChessMove);
    g_theEventSystem->SubscribeEventCallbackFunction("OnGameStateChanged", OnEnterMatchState);
    g_theEventSystem->SubscribeEventCallbackFunction("OnEnterMatchTurn", OnEnterMatchTurn);
    g_theEventSystem->SubscribeEventCallbackFunction("OnExitMatchTurn", OnExitMatchTurn);
    g_theEventSystem->SubscribeEventCallbackFunction("OnMatchInitialized", OnMatchInitialized);

    m_screenCamera = new Camera();

    Vec2 const bottomLeft     = Vec2::ZERO;
    Vec2 const screenTopRight = Vec2(Window::s_mainWindow->GetViewportDimensions().x, Window::s_mainWindow->GetViewportDimensions().y);

    m_screenCamera->SetOrthoGraphicView(bottomLeft, screenTopRight);
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    m_gameClock = new Clock(Clock::GetSystemClock());

    CreateBoard();

    for (PieceDefinition* pieceDef : PieceDefinition::s_pieceDefinitions)
    {
        pieceDef->CreateMeshByID(0);
        pieceDef->CreateMeshByID(1);
    }

    for (BoardDefinition const* boardDefs : BoardDefinition::s_boardDefinitions)
    {
        for (sSquareInfo const& squareInfo : boardDefs->m_squareInfos)
        {
            m_board->m_squareInfoList.push_back(squareInfo);

            if (squareInfo.m_name == "DEFAULT") continue;

            Piece* piece         = new Piece(this, squareInfo);
            piece->m_orientation = boardDefs->m_pieceOrientation;
            piece->m_color       = boardDefs->m_pieceColor;
            m_pieceList.push_back(piece);
        }
    }

    #if defined DEBUG_MODE
    DebugAddWorldBasis(Mat44(), -1.f);

    Mat44 transform;

    transform.SetIJKT3D(-Vec3::Y_BASIS, Vec3::X_BASIS, Vec3::Z_BASIS, Vec3(0.25f, 0.f, 0.25f));
    DebugAddWorldText("X-Forward", transform, 0.25f, Vec2::ONE, -1.f, Rgba8::RED);

    transform.SetIJKT3D(-Vec3::X_BASIS, -Vec3::Y_BASIS, Vec3::Z_BASIS, Vec3(0.f, 0.25f, 0.5f));
    DebugAddWorldText("Y-Left", transform, 0.25f, Vec2::ZERO, -1.f, Rgba8::GREEN);

    transform.SetIJKT3D(-Vec3::X_BASIS, Vec3::Z_BASIS, Vec3::Y_BASIS, Vec3(0.f, -0.25f, 0.25f));
    DebugAddWorldText("Z-Up", transform, 0.25f, Vec2(1.f, 0.f), -1.f, Rgba8::BLUE);
    #endif
}

//----------------------------------------------------------------------------------------------------
Match::~Match()
{
    UnregisterNetworkCommands();

    GAME_SAFE_RELEASE(m_screenCamera);
    GAME_SAFE_RELEASE(m_board);

    for (int i = 0; i < static_cast<int>(m_pieceList.size()); ++i)
    {
        GAME_SAFE_RELEASE(m_pieceList[i]);
    }

    m_pieceList.clear();
}

void Match::Update()
{
    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    DebugAddScreenText(Stringf("Time: %.2f\nFPS: %.2f\nScale: %.1f", m_gameClock->GetTotalSeconds(), 1.f / m_gameClock->GetDeltaSeconds(), m_gameClock->GetTimeScale()), m_screenCamera->GetOrthographicTopRight() - Vec2(250.f, 60.f), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);

    UpdateFromInput(deltaSeconds);

    m_board->Update(deltaSeconds);

    for (Piece* piece : m_pieceList)
    {
        if (piece == nullptr) continue;
        piece->Update(deltaSeconds);
    }

    // 檢查是否有任何 piece 或 board square 被選中
    bool hasAnySelection = false;
    // Piece*  selectedPiece        = nullptr;
    IntVec2 selectedSquareCoords = IntVec2::ZERO;
    bool    hasSelectedSquare    = false;

    // 檢查是否有 piece 被選中
    for (Piece* piece : m_pieceList)
    {
        if (piece != nullptr && piece->m_isSelected)
        {
            hasAnySelection = true;
            m_selectedPiece = piece;
            break;
        }
    }

    // 如果沒有 piece 被選中，檢查是否有 board square 被選中
    if (!hasAnySelection)
    {
        for (int i = 0; i < (int)m_board->m_squareInfoList.size(); ++i)
        {
            if (m_board->m_squareInfoList[i].m_isSelected)
            {
                hasAnySelection      = true;
                hasSelectedSquare    = true;
                selectedSquareCoords = m_board->m_squareInfoList[i].m_coords;
                m_selectedPiece      = m_board->GetPieceByCoords(selectedSquareCoords);
                break;
            }
        }
    }

    // 如果有選中的項目，進行 raycast 並檢查是否為有效移動位置
    if (hasAnySelection)
    {
        PlayerController* currentPlayer            = g_theGame->GetCurrentPlayer();
        EulerAngles       currentPlayerOrientation = currentPlayer->m_worldCamera->GetOrientation();

        Vec3 currentPlayerForwardNormal = currentPlayerOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();
        Ray3 ray                        = Ray3(currentPlayer->m_position, currentPlayerForwardNormal, 100.f);

        float minLength        = FLOAT_MAX;
        int   closestAABBIndex = -1;
        bool  foundImpact      = false;

        // 清除所有 highlight
        for (int i = 0; i < (int)m_board->m_squareInfoList.size(); ++i)
        {
            m_board->m_squareInfoList[i].m_isHighlighted = false;
        }

        for (Piece* piece : m_pieceList)
        {
            if (piece != nullptr)
            {
                piece->m_isHighlighted = false;
            }
        }

        // 清除 ghost render
        m_showGhostPiece   = false;
        m_ghostSourcePiece = nullptr;

        // Check board AABBs for raycast
        for (int i = 0; i < (int)m_board->m_squareInfoList.size(); ++i)
        {
            RaycastResult3D const result = RaycastVsAABB3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength,
                                                           m_board->GetAABB3FromCoords(m_board->m_squareInfoList[i].m_coords, 0.2f).m_mins,
                                                           m_board->GetAABB3FromCoords(m_board->m_squareInfoList[i].m_coords, 0.2f).m_maxs);

            if (result.m_didImpact && result.m_impactLength < minLength)
            {
                minLength        = result.m_impactLength;
                closestAABBIndex = i;
                foundImpact      = true;
            }
        }

        // 如果找到了 raycast 目標，檢查是否為有效移動位置
        if (foundImpact && closestAABBIndex != -1)
        {
            IntVec2 targetCoords = m_board->m_squareInfoList[closestAABBIndex].m_coords;
            IntVec2 fromCoords;
            bool    canHighlight = false;
            Piece*  sourcePiece  = nullptr;

            if (m_selectedPiece != nullptr)
            {
                // 有選中的棋子，檢查是否可以移動到目標位置
                fromCoords             = m_selectedPiece->m_coords;
                sourcePiece            = m_selectedPiece;
                eMoveResult moveResult = ValidateChessMove(fromCoords, targetCoords, "", m_isCheatMode);
                canHighlight           = (moveResult == eMoveResult::VALID_MOVE_NORMAL ||
                    moveResult == eMoveResult::VALID_CAPTURE_NORMAL ||
                    moveResult == eMoveResult::VALID_MOVE_PROMOTION ||
                    moveResult == eMoveResult::VALID_CAPTURE_ENPASSANT ||
                    moveResult == eMoveResult::VALID_CASTLE_KINGSIDE ||
                    moveResult == eMoveResult::VALID_CASTLE_QUEENSIDE);
            }
            else if (hasSelectedSquare)
            {
                // 有選中的方格，檢查該方格是否有棋子，以及是否可以移動到目標位置
                Piece const* pieceOnSelectedSquare = m_board->GetPieceByCoords(selectedSquareCoords);
                if (pieceOnSelectedSquare != nullptr)
                {
                    fromCoords             = selectedSquareCoords;
                    sourcePiece            = const_cast<Piece*>(pieceOnSelectedSquare);
                    eMoveResult moveResult = ValidateChessMove(fromCoords, targetCoords, "", m_isCheatMode);
                    canHighlight           = (moveResult == eMoveResult::VALID_MOVE_NORMAL ||
                        moveResult == eMoveResult::VALID_CAPTURE_NORMAL ||
                        moveResult == eMoveResult::VALID_MOVE_PROMOTION ||
                        moveResult == eMoveResult::VALID_CAPTURE_ENPASSANT ||
                        moveResult == eMoveResult::VALID_CASTLE_KINGSIDE ||
                        moveResult == eMoveResult::VALID_CASTLE_QUEENSIDE);
                }
            }

            // 如果是有效移動位置，只進行 highlight（不發送 ChessMove 事件）
            if (canHighlight && sourcePiece != nullptr)
            {
                m_board->m_squareInfoList[closestAABBIndex].m_isHighlighted = true;
                // 設置 ghost render
                m_showGhostPiece     = true;
                m_ghostSourcePiece   = sourcePiece;
                m_ghostPiecePosition = m_board->GetWorldPositionByCoords(targetCoords);
                m_ghostPiecePosition.z += 0.01f; // 稍微抬高一點避免 z-fighting
            }
        }
    }
    else
    {
        // 沒有任何選擇的情況下，清除 ghost render
        m_showGhostPiece   = false;
        m_ghostSourcePiece = nullptr;

        // 沒有任何選擇的情況下，進行正常的 raycast 和 highlighting
        PlayerController* currentPlayer            = g_theGame->GetCurrentPlayer();
        EulerAngles       currentPlayerOrientation = currentPlayer->m_worldCamera->GetOrientation();

        Vec3 currentPlayerForwardNormal = currentPlayerOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();
        Ray3 ray                        = Ray3(currentPlayer->m_position, currentPlayerForwardNormal, 100.f);

        float  minLength        = FLOAT_MAX;
        Piece* closestPiece     = nullptr;
        int    closestAABBIndex = -1;
        bool   foundImpact      = false;

        // Check board AABBs
        for (int i = 0; i < (int)m_board->m_squareInfoList.size(); ++i)
        {
            RaycastResult3D const result = RaycastVsAABB3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength,
                                                           m_board->GetAABB3FromCoords(m_board->m_squareInfoList[i].m_coords, 0.2f).m_mins,
                                                           m_board->GetAABB3FromCoords(m_board->m_squareInfoList[i].m_coords, 0.2f).m_maxs);

            if (result.m_didImpact && result.m_impactLength < minLength)
            {
                minLength        = result.m_impactLength;
                closestAABBIndex = i;
                closestPiece     = nullptr; // Clear piece selection
                foundImpact      = true;
            }
        }

        // Check pieces
        for (Piece* piece : m_pieceList)
        {
            RaycastResult3D result = RaycastVsCylinderZ3D(
                currentPlayer->m_position,
                currentPlayerForwardNormal,
                ray.m_maxLength,
                (piece->m_position + Vec3::Z_BASIS * 0.5f).GetXY(),
                FloatRange(piece->m_position.z, piece->m_position.z + 1.f),
                0.25f
            );

            if (result.m_didImpact && result.m_impactLength < minLength)
            {
                minLength        = result.m_impactLength;
                closestPiece     = piece;
                closestAABBIndex = -1; // Clear AABB selection
                foundImpact      = true;
            }
        }

        // Handle selections based on closest impact
        if (foundImpact)
        {
            // Clear all board selections first
            for (int i = 0; i < (int)m_board->m_squareInfoList.size(); ++i)
            {
                m_board->m_squareInfoList[i].m_isHighlighted = false;
            }

            // Set board selection if closest impact was an AABB
            if (closestAABBIndex != -1)
            {
                m_board->m_squareInfoList[closestAABBIndex].m_isHighlighted = true;
            }

            // Set piece selection
            for (Piece* piece : m_pieceList)
            {
                piece->m_isHighlighted = (piece == closestPiece);
            }
        }
        else
        {
            // No impacts found, clear all highlights
            for (int i = 0; i < (int)m_board->m_squareInfoList.size(); ++i)
            {
                m_board->m_squareInfoList[i].m_isHighlighted = false;
            }

            for (Piece* piece : m_pieceList)
            {
                piece->m_isHighlighted = false;
            }
        }
    }
}

void Match::UpdateFromInput(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
    if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
    {
        m_sunDirection.x -= 1.f;
        g_theLightSubsystem->GetLight(2)->SetDirection(m_sunDirection);
        DebugAddMessage(Stringf("Sun Direction: (%.2f, %.2f, %.2f)", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z), 5.f);
    }

    if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
    {
        m_sunDirection.x += 1.f;
        g_theLightSubsystem->GetLight(2)->SetDirection(m_sunDirection);
        DebugAddMessage(Stringf("Sun Direction: (%.2f, %.2f, %.2f)", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z), 5.f);
    }

    if (g_theInput->WasKeyJustPressed(KEYCODE_CONTROL))
    {
        m_isCheatMode = true;
    }
    if (g_theInput->WasKeyJustReleased(KEYCODE_CONTROL))
    {
        m_isCheatMode = false;
    }

    // 左鍵點擊處理
    if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
    {
        // 檢查是否有任何東西已經被選中
        bool    hasAnySelection      = false;
        Piece*  selectedPiece        = nullptr;
        IntVec2 selectedSquareCoords = IntVec2::ZERO;
        bool    hasSelectedSquare    = false;

        // 檢查 piece 選擇狀態
        for (Piece* piece : m_pieceList)
        {
            if (piece != nullptr && piece->m_isSelected)
            {
                if (piece->m_id == g_theGame->GetCurrentPlayerControllerId())
                {
                    hasAnySelection = true;
                    selectedPiece   = piece;
                    break;
                }
            }
        }

        // 檢查 board square 選擇狀態
        if (!hasAnySelection)
        {
            for (sSquareInfo& info : m_board->m_squareInfoList)
            {
                if (info.m_isSelected)
                {
                    hasAnySelection      = true;
                    hasSelectedSquare    = true;
                    selectedSquareCoords = info.m_coords;
                    break;
                }
            }
        }

        // 如果有選中的項目，檢查是否點擊了有效的移動目標
        if (hasAnySelection)
        {
            // 進行 raycast 來找到點擊的目標
            PlayerController* currentPlayer              = g_theGame->GetCurrentPlayer();
            EulerAngles       currentPlayerOrientation   = currentPlayer->m_worldCamera->GetOrientation();
            Vec3              currentPlayerForwardNormal = currentPlayerOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();
            Ray3              ray                        = Ray3(currentPlayer->m_position, currentPlayerForwardNormal, 100.f);

            float minLength        = FLT_MAX;
            int   closestAABBIndex = -1;
            bool  foundImpact      = false;

            // Check board AABBs for raycast
            for (int i = 0; i < (int)m_board->m_squareInfoList.size(); ++i)
            {
                RaycastResult3D const result = RaycastVsAABB3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength,
                                                               m_board->GetAABB3FromCoords(m_board->m_squareInfoList[i].m_coords, 0.2f).m_mins,
                                                               m_board->GetAABB3FromCoords(m_board->m_squareInfoList[i].m_coords, 0.2f).m_maxs);

                if (result.m_didImpact && result.m_impactLength < minLength)
                {
                    minLength        = result.m_impactLength;
                    closestAABBIndex = i;
                    foundImpact      = true;
                }
            }

            // 如果找到了點擊目標，檢查是否為有效移動並執行
            if (foundImpact && closestAABBIndex != -1)
            {
                IntVec2 targetCoords = m_board->m_squareInfoList[closestAABBIndex].m_coords;
                IntVec2 fromCoords;
                bool    canMove = false;

                if (selectedPiece != nullptr)
                {
                    // 有選中的棋子
                    fromCoords             = selectedPiece->m_coords;
                    eMoveResult moveResult = ValidateChessMove(fromCoords, targetCoords, "", m_isCheatMode);
                    canMove                = (moveResult == eMoveResult::VALID_MOVE_NORMAL ||
                        moveResult == eMoveResult::VALID_CAPTURE_NORMAL ||
                        moveResult == eMoveResult::VALID_MOVE_PROMOTION ||
                        moveResult == eMoveResult::VALID_CAPTURE_ENPASSANT ||
                        moveResult == eMoveResult::VALID_CASTLE_KINGSIDE ||
                        moveResult == eMoveResult::VALID_CASTLE_QUEENSIDE);
                }
                else if (hasSelectedSquare)
                {
                    // 有選中的方格
                    Piece const* pieceOnSelectedSquare = m_board->GetPieceByCoords(selectedSquareCoords);
                    if (pieceOnSelectedSquare != nullptr)
                    {
                        fromCoords             = selectedSquareCoords;
                        eMoveResult moveResult = ValidateChessMove(fromCoords, targetCoords, "", m_isCheatMode);
                        canMove                = (moveResult == eMoveResult::VALID_MOVE_NORMAL ||
                            moveResult == eMoveResult::VALID_CAPTURE_NORMAL ||
                            moveResult == eMoveResult::VALID_MOVE_PROMOTION ||
                            moveResult == eMoveResult::VALID_CAPTURE_ENPASSANT ||
                            moveResult == eMoveResult::VALID_CASTLE_KINGSIDE ||
                            moveResult == eMoveResult::VALID_CASTLE_QUEENSIDE);
                    }
                }

                // 如果是有效移動，發送 ChessMove 事件
                if (canMove)
                {
                    EventArgs args;
                    args.SetValue("from", m_board->ChessCoordToString(fromCoords));
                    args.SetValue("to", m_board->ChessCoordToString(targetCoords));
                    args.SetValue("promoteTo", "");
                    String teleport = m_isCheatMode ? "true" : "false";
                    args.SetValue("teleport", teleport);

                    g_theEventSystem->FireEvent("ChessMove", args);

                    // 移動完成後清除選擇
                    for (sSquareInfo& info : m_board->m_squareInfoList)
                    {
                        info.m_isSelected    = false;
                        info.m_isHighlighted = false;
                    }

                    for (Piece* piece : m_pieceList)
                    {
                        if (piece != nullptr)
                        {
                            piece->m_isSelected    = false;
                            piece->m_isHighlighted = false;
                        }
                    }
                }
            }
        }
        else
        {
            // 如果沒有任何選擇，允許選擇目前 highlighted 的項目
            for (sSquareInfo& info : m_board->m_squareInfoList)
            {
                if (info.m_isHighlighted && (m_isCheatMode || info.m_playerControllerId == g_theGame->GetCurrentPlayerControllerId()))
                {
                    info.m_isSelected = true;
                    // 選中後不保持 highlight，因為一旦選中就不允許其他項目 highlight
                    info.m_isHighlighted = false;
                }
            }

            for (Piece* piece : m_pieceList)
            {
                if (piece != nullptr && piece->m_isHighlighted && (m_isCheatMode || piece->m_id == g_theGame->GetCurrentPlayerControllerId()))
                {
                    piece->m_isSelected = true;
                    // 選中後不保持 highlight，因為一旦選中就不允許其他項目 highlight
                    piece->m_isHighlighted = false;
                }
            }
        }
    }

    // 右鍵點擊 - 用於取消選擇
    if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
    {
        // 清除所有選擇和 highlight
        for (sSquareInfo& info : m_board->m_squareInfoList)
        {
            info.m_isSelected    = false;
            info.m_isHighlighted = false;
        }

        for (Piece* piece : m_pieceList)
        {
            if (piece != nullptr)
            {
                piece->m_isSelected    = false;
                piece->m_isHighlighted = false;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Match::Render() const
{
    // g_theRenderer->SetLightConstants(Rgba8::WHITE, m_sunDirection, m_ambientIntensity, 8);

    m_board->Render();

    for (Piece const* piece : m_pieceList)
    {
        if (piece == nullptr) continue;

        piece->Render();
    }

    // 渲染 ghost piece（如果需要的話）
    if (m_showGhostPiece && m_ghostSourcePiece != nullptr)
    {
        RenderGhostPiece();
    }

    RenderPlayerBasis();
}

// 新增 RenderGhostPiece 方法
void Match::RenderGhostPiece() const
{
    if (m_ghostSourcePiece == nullptr) return;

    // 保存原始位置和渲染狀態
    Vec3 originalPosition      = m_ghostSourcePiece->m_position;
    bool originalIsSelected    = m_ghostSourcePiece->m_isSelected;
    bool originalIsHighlighted = m_ghostSourcePiece->m_isHighlighted;

    // 暫時修改棋子的位置和狀態用於 ghost 渲染
    m_ghostSourcePiece->m_position      = m_ghostPiecePosition;
    m_ghostSourcePiece->m_isSelected    = false;
    m_ghostSourcePiece->m_isHighlighted = false;

    // 渲染 ghost piece
    m_ghostSourcePiece->RenderTargetPiece();


    // 恢復原始位置和狀態
    m_ghostSourcePiece->m_position      = originalPosition;
    m_ghostSourcePiece->m_isSelected    = originalIsSelected;
    m_ghostSourcePiece->m_isHighlighted = originalIsHighlighted;
}

//----------------------------------------------------------------------------------------------------
void Match::CreateBoard()
{
    m_board = new Board(this);
}

void Match::ExecuteCapture(IntVec2 const& fromCoords,
                           IntVec2 const& toCoords,
                           String const&  promoteTo)
{
    Piece*       fromPiece = m_board->GetPieceByCoords(fromCoords);
    Piece const* toPiece   = m_board->GetPieceByCoords(toCoords);

    if (toPiece == nullptr) return;

    // If captured piece is a king, end the match
    if (toPiece->m_definition->m_type == ePieceType::KING)
    {
        fromPiece->UpdatePositionByCoords(toCoords);
        IsValidPromotionType(promoteTo) ? m_board->UpdateSquareInfoList(fromCoords, toCoords, promoteTo) : m_board->UpdateSquareInfoList(fromCoords, toCoords);
        RemovePieceFromPieceList(toPiece->m_coords);

        g_theDevConsole->AddLine(DevConsole::WARNING, "##################################################");
        g_theDevConsole->AddLine(DevConsole::WARNING, Stringf("[SYSTEM] Player #%d has won the match!", g_theGame->GetCurrentPlayerControllerId()));
        g_theDevConsole->AddLine(DevConsole::WARNING, "##################################################");
        g_theGame->ChangeGameState(eGameState::FINISHED);
    }
    else
    {
        fromPiece->UpdatePositionByCoords(toCoords);
        IsValidPromotionType(promoteTo) ? m_board->UpdateSquareInfoList(fromCoords, toCoords, promoteTo) : m_board->UpdateSquareInfoList(fromCoords, toCoords);
        RemovePieceFromPieceList(toPiece->m_coords);
    }
}

void Match::RemovePieceFromPieceList(IntVec2 const& toCoords)
{
    for (auto it = m_pieceList.begin(); it != m_pieceList.end();)
    {
        Piece* piece = *it;

        if (piece->m_coords == toCoords)
        {
            delete piece;

            it = m_pieceList.erase(it);
            break;
        }

        ++it;
    }
}

// bool Match::OnChessMove(EventArgs& args)
// {
//     String const from       = args.GetValue("from", "DEFAULT");
//     String const to         = args.GetValue("to", "DEFAULT");
//     String const promotion  = args.GetValue("promoteTo", "DEFAULT");
//     bool const   isTeleport = args.GetValue("teleport", false);
//
//     IntVec2 const fromCoords = g_theGame->m_match->m_board->StringToChessCoord(from);
//     IntVec2 const toCoords   = g_theGame->m_match->m_board->StringToChessCoord(to);
//
//     g_theGame->m_match->OnChessMove(fromCoords, toCoords, promotion, isTeleport);
//     return true;
// }

bool Match::OnEnterMatchState(EventArgs& args)
{
    OnEnterMatchTurn(args);
    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnEnterMatchTurn(EventArgs& args)
{
    UNUSED(args)

    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("=================================================="));
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("Player #%d -- it's your turn!", g_theGame->GetCurrentPlayerControllerId()));

    int const currentTurnPlayerIndex = g_theGame->GetCurrentPlayerControllerId();

    if (currentTurnPlayerIndex == 0 || currentTurnPlayerIndex == -1) g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Game state is: First Player's Turn"));
    else if (currentTurnPlayerIndex == 1) g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Game state is: Second Player's Turn"));

    g_theDevConsole->AddLine(DevConsole::INPUT_TEXT, Stringf("  ABCDEFGH"));
    g_theDevConsole->AddLine(DevConsole::INPUT_TEXT, Stringf(" +--------+"));

    for (int row = 8; row >= 1; --row)
    {
        g_theDevConsole->AddLine(DevConsole::INPUT_TEXT, Stringf("%d|%s|%d", row, g_theGame->m_match->m_board->GetBoardContents(row).c_str(), row));
    }

    g_theDevConsole->AddLine(DevConsole::INPUT_TEXT, Stringf(" +--------+"));
    g_theDevConsole->AddLine(DevConsole::INPUT_TEXT, Stringf("  ABCDEFGH"));

    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnExitMatchTurn(EventArgs& args)
{
    UNUSED(args)

    g_theGame->TogglePlayerControllerId();
    g_theEventSystem->FireEvent("OnEnterMatchTurn");
    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnMatchInitialized(EventArgs& args)
{
    UNUSED(args)
    g_theEventSystem->FireEvent("OnEnterMatchTurn");
    return true;
}

//----------------------------------------------------------------------------------------------------
void Match::OnChessMove(IntVec2 const& fromCoords,
                        IntVec2 const& toCoords,
                        String const&  promoteTo,
                        bool const     isTeleport)
{
    if (ExecuteMove(fromCoords, toCoords, promoteTo, isTeleport)) g_theEventSystem->FireEvent("OnExitMatchTurn");
}

eMoveResult Match::ValidateChessMove(IntVec2 const& fromCoords,
                                     IntVec2 const& toCoords,
                                     String const&  promotionType,
                                     bool const     isTeleport) const
{
    // 1. Check if coordinates are valid
    if (!m_board->IsCoordValid(fromCoords) || !m_board->IsCoordValid(toCoords))
    {
        return eMoveResult::INVALID_MOVE_BAD_LOCATION;
    }

    // 2. Check if source square has a piece
    Piece const* fromPiece = m_board->GetPieceByCoords(fromCoords);

    if (fromPiece == nullptr)
    {
        return eMoveResult::INVALID_MOVE_NO_PIECE;
    }

    // 3. Check if piece belongs to current player
    if (m_board->GetSquareInfoByCoords(fromCoords).m_playerControllerId != g_theGame->GetCurrentPlayerControllerId())
    {
        return eMoveResult::INVALID_MOVE_NOT_YOUR_PIECE;
    }

    // 4. Check if trying to move to same square
    if (fromCoords == toCoords)
    {
        return eMoveResult::INVALID_MOVE_ZERO_DISTANCE;
    }

    // 5. Check destination square
    Piece const* toPiece = m_board->GetPieceByCoords(toCoords);
    int const    toOwner = m_board->GetSquareInfoByCoords(toCoords).m_playerControllerId;

    if (toPiece != nullptr)
    {
        if (isTeleport)
        {
            return eMoveResult::VALID_MOVE_PROMOTION;
        }
        if (toOwner == g_theGame->GetCurrentPlayerControllerId()) return eMoveResult::INVALID_MOVE_DESTINATION_BLOCKED;
    }
    else
    {
        if (isTeleport) return eMoveResult::VALID_MOVE_NORMAL;
    }

    // 6. Check piece-specific movement rules
    eMoveResult const pieceValidation = ValidatePieceMovement(fromCoords, toCoords, promotionType);

    if (pieceValidation != eMoveResult::VALID_MOVE_NORMAL) return pieceValidation;

    // 7. Check if sliding pieces are blocked
    if (!IsPathClear(fromCoords, toCoords, fromPiece->m_definition->m_type))
    {
        return eMoveResult::INVALID_MOVE_PATH_BLOCKED;
    }

    // 8. Kings apart rule - king cannot move adjacent to enemy king
    if (fromPiece->m_definition->m_type == ePieceType::KING)
    {
        if (!IsKingDistanceValid(toCoords))
        {
            return eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
        }
    }

    // Determine the type of valid move
    return DetermineValidMoveType(fromCoords, toCoords, fromPiece);
}

eMoveResult Match::ValidatePieceMovement(IntVec2 const& fromCoords,
                                         IntVec2 const& toCoords,
                                         String const&  promotionType) const
{
    Piece const*     fromPiece = m_board->GetPieceByCoords(fromCoords);
    ePieceType const pieceType = fromPiece->m_definition->m_type;

    int const deltaX    = toCoords.x - fromCoords.x;
    int const deltaY    = toCoords.y - fromCoords.y;
    int const absDeltaX = abs(deltaX);
    int const absDeltaY = abs(deltaY);

    switch (pieceType)
    {
    case ePieceType::PAWN: return ValidatePawnMove(fromCoords, toCoords, promotionType);
    case ePieceType::ROOK: return ValidateRookMove(deltaX, deltaY);
    case ePieceType::BISHOP: return ValidateBishopMove(absDeltaX, absDeltaY);
    case ePieceType::KNIGHT: return ValidateKnightMove(absDeltaX, absDeltaY);
    case ePieceType::QUEEN: return ValidateQueenMove(deltaX, deltaY, absDeltaX, absDeltaY);
    case ePieceType::KING: return ValidateKingMove(absDeltaX, absDeltaY, fromCoords, toCoords);
    case ePieceType::NONE:
    default: return eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
    }
}

eMoveResult Match::ValidatePawnMove(IntVec2 const& fromCoords,
                                    IntVec2 const& toCoords,
                                    String const&  promotionType) const
{
    Piece const* fromPiece = m_board->GetPieceByCoords(fromCoords);
    Piece const* toPiece   = m_board->GetPieceByCoords(toCoords);

    int currentPlayer = g_theGame->GetCurrentPlayerControllerId();
    int direction     = (currentPlayer == 0) ? 1 : -1; // Player 0 moves up, Player 1 moves down

    int deltaX = toCoords.x - fromCoords.x;
    int deltaY = toCoords.y - fromCoords.y;

    // Check for pawn promotion
    int promotionRank = (currentPlayer == 0) ? 8 : 1;
    if (toCoords.y == promotionRank)
    {
        if (promotionType.empty())
        {
            return eMoveResult::VALID_MOVE_PROMOTION; // Need promotion parameter
        }

        if (!IsValidPromotionType(promotionType))
        {
            return eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
        }
    }

    // Forward movement (1 or 2 squares)
    if (deltaX == 0 && toPiece == nullptr)
    {
        if (deltaY == direction) // 1 square forward
        {
            return eMoveResult::VALID_MOVE_NORMAL;
        }
        else if (deltaY == 2 * direction) // 2 squares forward
        {
            // Check if pawn is in starting position or has never moved
            int startingRank = (currentPlayer == 0) ? 1 : 6;
            if (fromCoords.y == startingRank || !fromPiece->m_hasMoved)
            {
                return eMoveResult::VALID_MOVE_NORMAL;
            }
            else
            {
                return eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
            }
        }
    }
    // Diagonal capture
    else if (abs(deltaX) == 1 && deltaY == direction)
    {
        if (toPiece != nullptr)
        {
            return eMoveResult::VALID_CAPTURE_NORMAL; // Normal capture
        }
        else
        {
            // Check for en passant
            if (IsValidEnPassant(fromCoords, toCoords))
            {
                return eMoveResult::VALID_CAPTURE_ENPASSANT;
            }
            else
            {
                return eMoveResult::INVALID_ENPASSANT_STALE;
            }
        }
    }

    return eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

eMoveResult Match::ValidateRookMove(int deltaX, int deltaY) const
{
    if ((deltaX == 0 && deltaY != 0) || (deltaX != 0 && deltaY == 0))
    {
        return eMoveResult::VALID_MOVE_NORMAL;
    }

    return eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

eMoveResult Match::ValidateBishopMove(int absDeltaX, int absDeltaY) const
{
    if (absDeltaX == absDeltaY && absDeltaX > 0)
    {
        return eMoveResult::VALID_MOVE_NORMAL;
    }

    return eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

eMoveResult Match::ValidateKnightMove(int absDeltaX, int absDeltaY) const
{
    if ((absDeltaX == 2 && absDeltaY == 1) || (absDeltaX == 1 && absDeltaY == 2))
    {
        return eMoveResult::VALID_MOVE_NORMAL;
    }

    return eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

eMoveResult Match::ValidateQueenMove(int deltaX, int deltaY, int absDeltaX, int absDeltaY) const
{
    // Queen moves like rook or bishop
    bool isRookMove   = (deltaX == 0 && deltaY != 0) || (deltaX != 0 && deltaY == 0);
    bool isBishopMove = (absDeltaX == absDeltaY && absDeltaX > 0);

    if (isRookMove || isBishopMove)
    {
        return eMoveResult::VALID_MOVE_NORMAL;
    }

    return eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

eMoveResult Match::ValidateKingMove(int            absDeltaX, int absDeltaY,
                                    IntVec2 const& fromCoords,
                                    IntVec2 const& toCoords) const
{
    // Check for castling
    if (absDeltaY == 0 && absDeltaX == 3)
    {
        return ValidateCastling(fromCoords, toCoords);
    }

    if (absDeltaY == 0 && absDeltaX == 4)
    {
        return ValidateCastling(fromCoords, toCoords);
    }

    // Normal king move (1 square in any direction)
    if (absDeltaX <= 1 && absDeltaY <= 1)
    {
        return eMoveResult::VALID_MOVE_NORMAL;
    }

    return eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

bool Match::IsKingDistanceValid(IntVec2 const& toCoords) const
{
    // Find enemy king position
    int const     enemyPlayerControllerId = 1 - g_theGame->GetCurrentPlayerControllerId();
    IntVec2 const enemyKingCoords         = m_board->FindKingCoordsByPlayerId(enemyPlayerControllerId);

    // Check if destination is adjacent to enemy king
    int deltaX = abs(toCoords.x - enemyKingCoords.x);
    int deltaY = abs(toCoords.y - enemyKingCoords.y);

    if (deltaX <= 1 && deltaY <= 1 && !(deltaX == 0 && deltaY == 0))
    {
        return false; // Kings cannot be adjacent
    }

    return true;
}

bool Match::IsPathClear(IntVec2 const& fromCoords, IntVec2 const& toCoords, ePieceType const& pieceType) const
{
    // Knights don't need to clear path
    if (pieceType == ePieceType::KNIGHT)
    {
        return true;
    }

    // Kings moving 1 square don't need path check
    if (pieceType == ePieceType::KING)
    {
        int absDeltaX = abs(toCoords.x - fromCoords.x);
        int absDeltaY = abs(toCoords.y - fromCoords.y);
        if (absDeltaX <= 1 && absDeltaY <= 1)
        {
            return true;
        }
    }

    int deltaX = toCoords.x - fromCoords.x;
    int deltaY = toCoords.y - fromCoords.y;

    int stepX = (deltaX == 0) ? 0 : (deltaX > 0) ? 1 : -1;
    int stepY = (deltaY == 0) ? 0 : (deltaY > 0) ? 1 : -1;

    IntVec2 currentPos = fromCoords;
    currentPos.x += stepX;
    currentPos.y += stepY;

    while (currentPos != toCoords)
    {
        if (m_board->GetPieceByCoords(currentPos) != nullptr)
        {
            return false;
        }

        currentPos.x += stepX;
        currentPos.y += stepY;
    }

    return true;
}

bool Match::IsValidEnPassant(IntVec2 const& fromCoords, IntVec2 const& toCoords) const
{
    sPieceMove const lastMove = GetLastPieceMove();

    // Check if last move was a pawn moving 2 squares
    if (lastMove.piece == nullptr || lastMove.piece->m_definition->m_type != ePieceType::PAWN)
    {
        return false;
    }

    int lastMoveDelta = abs(lastMove.toCoords.y - lastMove.fromCoords.y);
    if (lastMoveDelta != 2)
    {
        return false;
    }

    // Check if the pawn to be captured is adjacent and the target square is the "passed through" square
    IntVec2 capturedPawnPos = IntVec2(toCoords.x, fromCoords.y);
    if (lastMove.toCoords == capturedPawnPos)
    {
        IntVec2 passedThroughSquare = IntVec2(lastMove.fromCoords.x, (lastMove.fromCoords.y + lastMove.toCoords.y) / 2);
        if (toCoords == passedThroughSquare)
        {
            return true;
        }
    }

    return false;
}

eMoveResult Match::ValidateCastling(IntVec2 const& fromCoords, IntVec2 const& toCoords) const
{
    Piece const* king = m_board->GetPieceByCoords(fromCoords);

    // King must not have moved
    if (king->m_hasMoved)
    {
        return eMoveResult::INVALID_CASTLE_KING_HAS_MOVED;
    }

    // Determine castling side
    bool    isKingSide = toCoords.x > fromCoords.x;
    IntVec2 rookPos    = IntVec2(isKingSide ? 8 : 1, fromCoords.y);

    Piece const* rook = m_board->GetPieceByCoords(rookPos);
    if (rook == nullptr || rook->m_definition->m_type != ePieceType::ROOK)
    {
        return eMoveResult::INVALID_CASTLE_ROOK_HAS_MOVED;
    }

    // Rook must not have moved
    if (rook->m_hasMoved)
    {
        return eMoveResult::INVALID_CASTLE_ROOK_HAS_MOVED;
    }

    // Check if path is clear between king and rook
    int startX = std::min(fromCoords.x, rookPos.x) + 1;
    int endX   = std::max(fromCoords.x, rookPos.x);

    for (int x = startX; x < endX; ++x)
    {
        if (m_board->GetPieceByCoords(IntVec2(x, fromCoords.y)) != nullptr)
        {
            return eMoveResult::INVALID_CASTLE_PATH_BLOCKED;
        }
    }

    // TODO: Add check-related castle validation in future assignments
    // - King cannot be in check
    // - King cannot pass through check
    // - King cannot end in check

    return isKingSide ? eMoveResult::VALID_CASTLE_KINGSIDE : eMoveResult::VALID_CASTLE_QUEENSIDE;
}

bool Match::IsValidPromotionType(String const& promoteTo) const
{
    return
        promoteTo == "queen" ||
        promoteTo == "rook" ||
        promoteTo == "bishop" ||
        promoteTo == "knight";
}

eMoveResult Match::DetermineValidMoveType(IntVec2 const& fromCoords,
                                          IntVec2 const& toCoords,
                                          Piece const*   fromPiece) const
{
    Piece const* toPiece = m_board->GetPieceByCoords(toCoords);

    // Check for pawn promotion
    if (fromPiece->m_definition->m_type == ePieceType::PAWN)
    {
        int currentPlayer = g_theGame->GetCurrentPlayerControllerId();
        int promotionRank = (currentPlayer == 0) ? 8 : 1;

        if (toCoords.y == promotionRank)
        {
            if (toPiece != nullptr)
            {
                return eMoveResult::VALID_MOVE_PROMOTION; // Promotion with capture
            }

            return eMoveResult::VALID_MOVE_PROMOTION; // Promotion without capture
        }

        // Check for en passant (already validated in pawn move)
        if (abs(toCoords.x - fromCoords.x) == 1 && toPiece == nullptr)
        {
            return eMoveResult::VALID_CAPTURE_ENPASSANT;
        }
    }

    // Check for castling
    if (fromPiece->m_definition->m_type == ePieceType::KING)
    {
        int absDeltaX = abs(toCoords.x - fromCoords.x);
        if (absDeltaX == 2)
        {
            bool isKingSide = toCoords.x > fromCoords.x;
            return isKingSide ? eMoveResult::VALID_CASTLE_KINGSIDE : eMoveResult::VALID_CASTLE_QUEENSIDE;
        }
    }

    // Check for capture
    if (toPiece != nullptr)
    {
        return eMoveResult::VALID_CAPTURE_NORMAL;
    }

    // Normal move
    return eMoveResult::VALID_MOVE_NORMAL;
}

sPieceMove Match::GetLastPieceMove() const
{
    if (m_pieceMoveList.size() == 0) return sPieceMove{};
    return m_pieceMoveList.back();
}

bool Match::ExecuteMove(IntVec2 const& fromCoords,
                        IntVec2 const& toCoords,
                        String const&  promoteTo,
                        bool const     isTeleport)
{
    eMoveResult const result = ValidateChessMove(fromCoords, toCoords, promoteTo, isTeleport);

    if (!IsMoveValid(result))
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, GetMoveResultString(result));
        return false;
    }

    Piece* fromPiece = m_board->GetPieceByCoords(fromCoords);
    g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Move Player #%d's %s from %s to %s", g_theGame->GetCurrentPlayerControllerId(), m_board->GetPieceByCoords(fromCoords)->m_definition->m_name.c_str(), m_board->ChessCoordToString(fromCoords).c_str(),
                                                             m_board->ChessCoordToString(toCoords).c_str()));
    switch (result)
    {
    case eMoveResult::VALID_CAPTURE_ENPASSANT: ExecuteEnPassantCapture(fromCoords, toCoords);
        break;
    case eMoveResult::VALID_MOVE_PROMOTION: ExecutePawnPromotion(fromCoords, toCoords, promoteTo);
        break;
    case eMoveResult::VALID_CASTLE_KINGSIDE: ExecuteKingsideCastling(fromCoords);
        break;
    case eMoveResult::VALID_CASTLE_QUEENSIDE: ExecuteQueensideCastling(fromCoords);
        break;
    case eMoveResult::VALID_CAPTURE_NORMAL: ExecuteCapture(fromCoords, toCoords);
        break;
    case eMoveResult::VALID_MOVE_NORMAL:
    default:
        fromPiece->UpdatePositionByCoords(toCoords, 2.f);
        fromPiece->m_hasMoved = true;
        m_board->UpdateSquareInfoList(fromCoords, toCoords);

        break;
    }

    // Record move for en passant detection
    m_pieceMoveList.push_back({fromPiece, fromCoords, toCoords});

    g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, GetMoveResultString(result));
    return true;
}

void Match::ExecuteEnPassantCapture(IntVec2 const& fromCoords, IntVec2 const& toCoords)
{
    // Remove the captured pawn
    IntVec2 capturedPawnPos = IntVec2(toCoords.x, fromCoords.y);
    Piece*  fromPiece       = m_board->GetPieceByCoords(fromCoords);
    fromPiece->UpdatePositionByCoords(toCoords);
    m_board->UpdateSquareInfoList(fromCoords, toCoords);
    m_board->UpdateSquareInfoList(capturedPawnPos);
    RemovePieceFromPieceList(capturedPawnPos);
    // Move the capturing pawn
    // m_board->MovePiece(fromCoords, toCoords);
}

void Match::ExecutePawnPromotion(IntVec2 const& fromCoords,
                                 IntVec2 const& toCoords,
                                 String const&  promoteTo)
{
    // Handle capture if there's a piece at destination

    Piece* fromPiece        = m_board->GetPieceByCoords(fromCoords);
    fromPiece->m_definition = PieceDefinition::GetDefByName(promoteTo);
    // fromPiece->UpdatePositionByCoords(toCoords);
    ExecuteCapture(fromCoords, toCoords);

    // m_board->UpdateSquareInfoList(fromCoords, toCoords, promoteTo);
}

void Match::ExecuteCastling(IntVec2 const& fromCoords,
                            IntVec2 const& toCoords) const
{
    bool const    isKingSide     = toCoords.x > fromCoords.x;
    IntVec2 const kingToCoords   = IntVec2(isKingSide ? 7 : 3, fromCoords.y);
    IntVec2 const rookFromCoords = IntVec2(isKingSide ? 8 : 1, fromCoords.y);
    IntVec2 const rookToCoords   = IntVec2(isKingSide ? 6 : 4, fromCoords.y);

    Piece* king = m_board->GetPieceByCoords(fromCoords);
    king->UpdatePositionByCoords(kingToCoords);
    m_board->UpdateSquareInfoList(fromCoords, kingToCoords);

    // Move rook
    Piece* rook = m_board->GetPieceByCoords(rookFromCoords);
    rook->UpdatePositionByCoords(rookToCoords);
    m_board->UpdateSquareInfoList(rookFromCoords, rookToCoords);
}

void Match::ExecuteKingsideCastling(IntVec2 const& fromCoords) const
{
    IntVec2 const kingToCoords   = IntVec2(7, fromCoords.y);
    IntVec2 const rookFromCoords = IntVec2(8, fromCoords.y);
    IntVec2 const rookToCoords   = IntVec2(6, fromCoords.y);

    Piece* king = m_board->GetPieceByCoords(fromCoords);
    king->UpdatePositionByCoords(kingToCoords);
    m_board->UpdateSquareInfoList(fromCoords, kingToCoords);

    // Move rook
    Piece* rook = m_board->GetPieceByCoords(rookFromCoords);
    rook->UpdatePositionByCoords(rookToCoords);
    m_board->UpdateSquareInfoList(rookFromCoords, rookToCoords);
}

void Match::ExecuteQueensideCastling(IntVec2 const& fromCoords) const
{
    IntVec2 const kingToCoords   = IntVec2(3, fromCoords.y);
    IntVec2 const rookFromCoords = IntVec2(1, fromCoords.y);
    IntVec2 const rookToCoords   = IntVec2(4, fromCoords.y);

    Piece* king = m_board->GetPieceByCoords(fromCoords);
    king->UpdatePositionByCoords(kingToCoords);
    m_board->UpdateSquareInfoList(fromCoords, kingToCoords);

    // Move rook
    Piece* rook = m_board->GetPieceByCoords(rookFromCoords);
    rook->UpdatePositionByCoords(rookToCoords);
    m_board->UpdateSquareInfoList(rookFromCoords, rookToCoords);
}

void Match::RenderPlayerBasis() const
{
    VertexList_PCU verts;

    Vec3 const worldCameraPosition = g_theGame->GetCurrentPlayer()->m_worldCamera->GetPosition();
    Vec3 const forwardNormal       = g_theGame->GetCurrentPlayer()->m_worldCamera->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();

    // Add vertices in world space.
    AddVertsForArrow3D(verts, worldCameraPosition + forwardNormal, worldCameraPosition + forwardNormal + Vec3::X_BASIS * 0.1f, 0.8f, 0.001f, 0.003f, Rgba8::RED);
    AddVertsForArrow3D(verts, worldCameraPosition + forwardNormal, worldCameraPosition + forwardNormal + Vec3::Y_BASIS * 0.1f, 0.8f, 0.001f, 0.003f, Rgba8::GREEN);
    AddVertsForArrow3D(verts, worldCameraPosition + forwardNormal, worldCameraPosition + forwardNormal + Vec3::Z_BASIS * 0.1f, 0.8f, 0.001f, 0.003f, Rgba8::BLUE);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::OPAQUE);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

STATIC bool Match::OnGameDataReceived(EventArgs& args)
{
    std::string data         = args.GetValue("data", "");
    int         fromClientId = args.GetValue("fromClientId", -1);

    // 清理顯示的資料
    std::string safeData;
    for (char c : data)
    {
        if ((c >= 32 && c <= 126) || c == ' ')
        {
            safeData += c;
        }
        else
        {
            safeData += '?'; // 替換不安全的字符
        }
    }

    if (g_theDevConsole)
    {
        if (fromClientId != -1)
        {
            g_theDevConsole->AddLine(Rgba8(255, 255, 255),
                                     Stringf("[MATCH] *** RECEIVED GAME DATA *** from client %d: '%s'", fromClientId, safeData.c_str()));
        }
        else
        {
            g_theDevConsole->AddLine(Rgba8(255, 255, 255),
                                     Stringf("[MATCH] *** RECEIVED GAME DATA *** from server: '%s'", safeData.c_str()));
        }

        // g_theGame->m_match->

    }
    return true;
}

//----------------------------------------------------------------------------------------------------
void Match::RegisterNetworkCommands()
{
    g_theEventSystem->SubscribeEventCallbackFunction("ChessServerInfo", OnChessServerInfo);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessListen", OnChessListen);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessConnect", OnChessConnect);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessDisconnect", OnChessDisconnect);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessPlayerInfo", OnChessPlayerInfo);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessBegin", OnChessBegin);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessValidate", OnChessValidate);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessMove", OnChessMove);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessResign", OnChessResign);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessOfferDraw", OnChessOfferDraw);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessAcceptDraw", OnChessAcceptDraw);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessRejectDraw", OnChessRejectDraw);
    g_theEventSystem->SubscribeEventCallbackFunction("RemoteCmd", OnRemoteCmd);
}

//----------------------------------------------------------------------------------------------------
void Match::UnregisterNetworkCommands()
{
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessServerInfo", OnChessServerInfo);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessListen", OnChessListen);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessConnect", OnChessConnect);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessDisconnect", OnChessDisconnect);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessPlayerInfo", OnChessPlayerInfo);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessBegin", OnChessBegin);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessValidate", OnChessValidate);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessMove", OnChessMove);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessResign", OnChessResign);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessOfferDraw", OnChessOfferDraw);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessAcceptDraw", OnChessAcceptDraw);
    g_theEventSystem->UnsubscribeEventCallbackFunction("ChessRejectDraw", OnChessRejectDraw);
    g_theEventSystem->UnsubscribeEventCallbackFunction("RemoteCmd", OnRemoteCmd);
}

//----------------------------------------------------------------------------------------------------
// NETWORK PROTOCOL EVENT HANDLERS
//----------------------------------------------------------------------------------------------------

bool Match::OnChessServerInfo(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    std::string ip = args.GetValue("ip", "");
    int port = args.GetValue("port", -1);

    // If connected, reject changes and warn
    if (match->m_isConnected)
    {
        g_theDevConsole->AddLine(DevConsole::WARNING,
            "Cannot change server info while connected. Disconnect first.");

        // Still show current values
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
            Stringf("Current settings - IP: %s, Port: %d, Connected: %s, Player: %s",
                match->m_serverIP.c_str(), match->m_serverPort,
                match->m_isConnected ? "YES" : "NO", match->m_myPlayerName.c_str()));
        return false;
    }

    // Update values if provided
    if (!ip.empty())
    {
        match->m_serverIP = ip;
    }
    if (port > 0)
    {
        match->m_serverPort = port;
    }

    // Print current values
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
        Stringf("Chess Server Info - IP: %s, Port: %d, Connected: %s, Player: %s, Game State: %s",
            match->m_serverIP.c_str(), match->m_serverPort,
            match->m_isConnected ? "YES" : "NO", match->m_myPlayerName.c_str(),
            match->GetGameState() == eChessGameState::PLAYER1_MOVING ? "Player1Moving" :
            match->GetGameState() == eChessGameState::PLAYER2_MOVING ? "Player2Moving" :
            match->GetGameState() == eChessGameState::GAME_OVER ? "GameOver" : "Waiting"));

    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessListen(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    int port = args.GetValue("port", match->m_serverPort);
    match->m_serverPort = port;

    bool success = g_theNetworkSubsystem->StartServer(port);
    if (success)
    {
        match->m_isServer = true;
        match->m_gameState = eChessGameState::WAITING_FOR_OPPONENT;
        g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
            Stringf("Chess server listening on port %d", port));
    }
    else
    {
        g_theDevConsole->AddLine(DevConsole::ERROR,
            Stringf("Failed to start chess server on port %d", port));
    }

    return success;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessConnect(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    std::string ip = args.GetValue("ip", match->m_serverIP);
    int port = args.GetValue("port", match->m_serverPort);

    match->m_serverIP = ip;
    match->m_serverPort = port;

    bool success = g_theNetworkSubsystem->ConnectToServer(ip, port);
    if (success)
    {
        match->m_isServer = false;
        match->m_isConnected = true;
        match->m_gameState = eChessGameState::WAITING_FOR_OPPONENT;
        g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
            Stringf("Connecting to chess server at %s:%d", ip.c_str(), port));
    }
    else
    {
        g_theDevConsole->AddLine(DevConsole::ERROR,
            Stringf("Failed to connect to chess server at %s:%d", ip.c_str(), port));
    }

    return success;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessDisconnect(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    std::string reason = args.GetValue("reason", "");
    bool isRemote = args.GetValue("remote", false);

    if (isRemote)
    {
        // Received disconnect from opponent
        g_theDevConsole->AddLine(DevConsole::WARNING,
            Stringf("Opponent disconnected. Reason: %s", reason.c_str()));

        // Just disconnect locally, don't send back
        if (match->m_isServer)
        {
            g_theNetworkSubsystem->StopServer();
        }
        else
        {
            g_theNetworkSubsystem->DisconnectFromServer();
        }
    }
    else
    {
        // Local disconnect request
        match->SendChessCommand(Stringf("ChessDisconnect reason=\"%s\"", reason.c_str()));

        // Disconnect after sending
        if (match->m_isServer)
        {
            g_theNetworkSubsystem->StopServer();
        }
        else
        {
            g_theNetworkSubsystem->DisconnectFromServer();
        }
    }

    match->m_isConnected = false;
    match->m_gameState = eChessGameState::WAITING_FOR_CONNECTION;
    match->m_player1Name = "";
    match->m_player2Name = "";

    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessPlayerInfo(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    std::string name = args.GetValue("name", "");
    bool isRemote = args.GetValue("remote", false);

    if (isRemote)
    {
        // Set opponent's name
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
            Stringf("Opponent player name: %s", name.c_str()));
        // We'll set the specific player slot when ChessBegin is called
    }
    else
    {
        // Set my name and send to opponent
        match->m_myPlayerName = name;
        match->SendChessCommand(Stringf("ChessPlayerInfo name=%s", name.c_str()));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
            Stringf("My player name set to: %s", name.c_str()));
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessBegin(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    std::string firstPlayer = args.GetValue("firstPlayer", "");
    bool isRemote = args.GetValue("remote", false);

    if (!isRemote)
    {
        // Local command - if no firstPlayer specified, default to me
        if (firstPlayer.empty())
        {
            firstPlayer = match->m_myPlayerName;
        }
        match->SendChessCommand(Stringf("ChessBegin firstPlayer=%s", firstPlayer.c_str()));
    }

    // Set up the game
    match->m_player1Name = firstPlayer;
    match->m_amIPlayer1 = (firstPlayer == match->m_myPlayerName);

    // Set player2 name to the other player
    if (match->m_amIPlayer1)
    {
        // I'm player1, so opponent is player2
        match->m_player2Name = "Opponent"; // This should be set by ChessPlayerInfo
    }
    else
    {
        // Opponent is player1, I'm player2
        match->m_player2Name = match->m_myPlayerName;
    }

    match->m_gameState = eChessGameState::PLAYER1_MOVING;
    match->m_moveNumber = 0;
    match->m_isConnected = true;

    // Reset board to starting position
    // You may want to implement a ResetToStartingPosition() method on your board

    g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
        Stringf("Chess game begun! %s (white) vs %s (black). %s to move.",
            match->m_player1Name.c_str(), match->m_player2Name.c_str(),
            match->m_player1Name.c_str()));

    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessValidate(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    bool isRemote = args.GetValue("remote", false);

    if (!isRemote)
    {
        // Send validation request to opponent
        std::string stateStr = match->GetGameState() == eChessGameState::PLAYER1_MOVING ? "Player1Moving" :
                              match->GetGameState() == eChessGameState::PLAYER2_MOVING ? "Player2Moving" :
                              "GameOver";

        std::string boardStr = match->GetBoardStateString();

        match->SendChessCommand(Stringf("ChessValidate state=%s player1=%s player2=%s move=%d board=%s",
            stateStr.c_str(), match->m_player1Name.c_str(), match->m_player2Name.c_str(),
            match->m_moveNumber, boardStr.c_str()));
    }
    else
    {
        // Validate received data against our state
        std::string state = args.GetValue("state", "");
        std::string player1 = args.GetValue("player1", "");
        std::string player2 = args.GetValue("player2", "");
        int move = args.GetValue("move", -1);
        std::string board = args.GetValue("board", "");

        if (!match->ValidateGameState(state, player1, player2, move, board))
        {
            match->DisconnectWithReason("VALIDATION FAILED");
            return false;
        }
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessMove(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    String const from = args.GetValue("from", "DEFAULT");
    String const to = args.GetValue("to", "DEFAULT");
    String const promotion = args.GetValue("promoteTo", "DEFAULT");
    bool const isTeleport = args.GetValue("teleport", false);
    bool isRemote = args.GetValue("remote", false);

    if (from == "DEFAULT" || to == "DEFAULT")
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, "ChessMove requires from= and to= parameters");
        return false;
    }

    IntVec2 const fromCoords = match->m_board->StringToChessCoord(from);
    IntVec2 const toCoords = match->m_board->StringToChessCoord(to);

    // Validate the move
    eMoveResult result = match->ValidateChessMove(fromCoords, toCoords, promotion, isTeleport);
    if (!IsMoveValid(result))
    {
        g_theDevConsole->AddLine(DevConsole::ERROR,
            Stringf("Invalid move: %s", GetMoveResultString(result)));
        return false;
    }

    // Check if it's the right player's turn
    bool shouldBeMyTurn = match->IsMyTurn();
    if (!isRemote && !shouldBeMyTurn && !isTeleport)
    {
        g_theDevConsole->AddLine(DevConsole::WARNING, "Not your turn!");
        return false;
    }

    // If this is a local move, send it to opponent
    if (!isRemote)
    {
        std::string command = Stringf("ChessMove from=%s to=%s", from.c_str(), to.c_str());
        if (promotion != "DEFAULT" && !promotion.empty())
        {
            command += Stringf(" promoteTo=%s", promotion.c_str());
        }
        if (isTeleport)
        {
            command += " teleport=true";
        }
        match->SendChessCommand(command);
    }

    // Execute the move
    match->OnChessMove(fromCoords, toCoords, promotion, isTeleport);

    // Update move counter and game state
    if (!isTeleport)
    {
        match->m_moveNumber++;
        match->m_gameState = (match->m_gameState == eChessGameState::PLAYER1_MOVING) ?
                            eChessGameState::PLAYER2_MOVING : eChessGameState::PLAYER1_MOVING;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessResign(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    bool isRemote = args.GetValue("remote", false);

    if (isRemote)
    {
        g_theDevConsole->AddLine(DevConsole::WARNING, "Opponent has resigned! You win!");
    }
    else
    {
        match->SendChessCommand("ChessResign");
        g_theDevConsole->AddLine(DevConsole::WARNING, "You have resigned. Game over.");
    }

    match->m_gameState = eChessGameState::GAME_OVER;
    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessOfferDraw(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    bool isRemote = args.GetValue("remote", false);

    if (isRemote)
    {
        // Received draw offer from opponent
        match->m_drawOfferFromOpponent = true;
        g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
            "Opponent offers a draw! Type 'ChessAcceptDraw' or 'ChessRejectDraw'");
    }
    else
    {
        // Local draw offer
        if (match->m_drawOffered)
        {
            g_theDevConsole->AddLine(DevConsole::WARNING,
                "You have already offered a draw this turn");
            return false;
        }

        match->SendChessCommand("ChessOfferDraw");
        match->m_drawOffered = true;
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Draw offer sent to opponent");
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessAcceptDraw(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    bool isRemote = args.GetValue("remote", false);

    if (isRemote)
    {
        g_theDevConsole->AddLine(DevConsole::WARNING,
            "Opponent accepted your draw offer! Game ends in a draw.");
    }
    else
    {
        if (!match->m_drawOfferFromOpponent)
        {
            g_theDevConsole->AddLine(DevConsole::ERROR,
                "No draw offer to accept from opponent");
            return false;
        }

        match->SendChessCommand("ChessAcceptDraw");
        g_theDevConsole->AddLine(DevConsole::WARNING,
            "You accepted the draw offer! Game ends in a draw.");
    }

    match->m_gameState = eChessGameState::GAME_OVER;
    match->m_drawOffered = false;
    match->m_drawOfferFromOpponent = false;
    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnChessRejectDraw(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    bool isRemote = args.GetValue("remote", false);

    if (isRemote)
    {
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
            "Opponent rejected your draw offer. Game continues.");
        match->m_drawOffered = false;
    }
    else
    {
        if (!match->m_drawOfferFromOpponent)
        {
            g_theDevConsole->AddLine(DevConsole::ERROR,
                "No draw offer to reject from opponent");
            return false;
        }

        match->SendChessCommand("ChessRejectDraw");
        match->m_drawOfferFromOpponent = false;
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
            "You rejected the draw offer. Game continues.");
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnRemoteCmd(EventArgs& args)
{
    Match* match = g_theGame->m_match;
    if (!match) return false;

    std::string cmd = args.GetValue("cmd", "");
    if (cmd.empty())
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, "RemoteCmd requires cmd= parameter");
        return false;
    }

    // Build command string with all other parameters
    std::string commandToSend = cmd;

    // Add all other arguments except 'cmd'
    for (std::pair<const std::string, std::string>& pair : args.GetAllKeyValuePairs())
    {
        if (pair.first != "cmd")
        {
            commandToSend += Stringf(" %s=%s", pair.first.c_str(), pair.second.c_str());
        }
    }

    match->SendChessCommand(commandToSend);
    return true;
}

//----------------------------------------------------------------------------------------------------
// HELPER FUNCTIONS
//----------------------------------------------------------------------------------------------------

void Match::SendChessCommand(const std::string& command)
{
    if (!m_isConnected)
    {
        g_theDevConsole->AddLine(DevConsole::WARNING, "Not connected to opponent");
        return;
    }

    if (!g_theNetworkSubsystem)
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, "Network subsystem not available");
        return;
    }

    g_theNetworkSubsystem->SendRawData(command);
}

//----------------------------------------------------------------------------------------------------
std::string Match::GetBoardStateString() const
{
    // Convert board to 64-character string representation
    // This needs to be implemented based on your board representation
    // Standard notation: RNBKQBNR for back rank, PPPPPPPP for pawns, etc.
    std::string boardStr = "";

    // Assuming your board has a method to get piece at coordinates
    for (int row = 8; row >= 1; --row)  // Chess rows 8 to 1
    {
        for (int col = 1; col <= 8; ++col)  // Chess columns a-h (1-8)
        {
            IntVec2 coords(col, row);
            Piece const* piece = m_board->GetPieceByCoords(coords);

            if (piece == nullptr)
            {
                boardStr += '.';
            }
            else
            {
                // Convert piece type to standard notation
                switch (piece->m_definition->m_type)
                {
                case ePieceType::PAWN:   boardStr += (piece->m_id == 0 ? 'P' : 'p'); break;
                case ePieceType::ROOK:   boardStr += (piece->m_id == 0 ? 'R' : 'r'); break;
                case ePieceType::KNIGHT: boardStr += (piece->m_id == 0 ? 'N' : 'n'); break;
                case ePieceType::BISHOP: boardStr += (piece->m_id == 0 ? 'B' : 'b'); break;
                case ePieceType::QUEEN:  boardStr += (piece->m_id == 0 ? 'Q' : 'q'); break;
                case ePieceType::KING:   boardStr += (piece->m_id == 0 ? 'K' : 'k'); break;
                default: boardStr += '.'; break;
                }
            }
        }
    }

    return boardStr;
}

//----------------------------------------------------------------------------------------------------
bool Match::ValidateGameState(const std::string& state, const std::string& player1,
                             const std::string& player2, int move, const std::string& board)
{
    bool isValid = true;
    std::string report = "=== CHESS VALIDATION REPORT ===\n";

    // Validate state
    std::string myState = m_gameState == eChessGameState::PLAYER1_MOVING ? "Player1Moving" :
                         m_gameState == eChessGameState::PLAYER2_MOVING ? "Player2Moving" :
                         "GameOver";
    if (state != myState)
    {
        report += Stringf("STATE MISMATCH: Expected %s, got %s\n", myState.c_str(), state.c_str());
        isValid = false;
    }

    // Validate player names
    if (player1 != m_player1Name)
    {
        report += Stringf("PLAYER1 MISMATCH: Expected %s, got %s\n", m_player1Name.c_str(), player1.c_str());
        isValid = false;
    }
    if (player2 != m_player2Name)
    {
        report += Stringf("PLAYER2 MISMATCH: Expected %s, got %s\n", m_player2Name.c_str(), player2.c_str());
        isValid = false;
    }

    // Validate move number
    if (move != m_moveNumber)
    {
        report += Stringf("MOVE NUMBER MISMATCH: Expected %d, got %d\n", m_moveNumber, move);
        isValid = false;
    }

    // Validate board state
    std::string myBoard = GetBoardStateString();
    if (board != myBoard)
    {
        report += Stringf("BOARD STATE MISMATCH:\nExpected: %s\nReceived: %s\n", myBoard.c_str(), board.c_str());
        isValid = false;
    }

    if (!isValid)
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, report);
    }
    else
    {
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Game state validation passed");
    }

    return isValid;
}

//----------------------------------------------------------------------------------------------------
void Match::DisconnectWithReason(const std::string& reason)
{
    SendChessCommand(Stringf("ChessDisconnect reason=\"%s\"", reason.c_str()));

    if (m_isServer)
    {
        g_theNetworkSubsystem->StopServer();
    }
    else
    {
        g_theNetworkSubsystem->DisconnectFromServer();
    }

    m_isConnected = false;
    m_gameState = eChessGameState::WAITING_FOR_CONNECTION;
}

//----------------------------------------------------------------------------------------------------
bool Match::IsMyTurn() const
{
    if (m_gameState == eChessGameState::PLAYER1_MOVING && m_amIPlayer1) return true;
    if (m_gameState == eChessGameState::PLAYER2_MOVING && !m_amIPlayer1) return true;
    return false;
}
