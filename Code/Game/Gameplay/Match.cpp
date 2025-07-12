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
    g_theEventSystem->SubscribeEventCallbackFunction("ChessMove", OnChessMove);
    g_theEventSystem->SubscribeEventCallbackFunction("OnGameStateChanged", OnEnterMatchState);
    g_theEventSystem->SubscribeEventCallbackFunction("OnEnterMatchTurn", OnEnterMatchTurn);
    g_theEventSystem->SubscribeEventCallbackFunction("OnExitMatchTurn", OnExitMatchTurn);
    g_theEventSystem->SubscribeEventCallbackFunction("OnMatchInitialized", OnMatchInitialized);
    // g_theEventSystem->SubscribeEventCallbackFunction("net_send_test", OnGameDataReceived);

    m_screenCamera = new Camera();

    Vec2 const bottomLeft     = Vec2::ZERO;
    Vec2 const screenTopRight = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);

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

    // #if defined DEBUG_MODE
    DebugAddWorldBasis(Mat44(), -1.f);

    Mat44 transform;

    transform.SetIJKT3D(-Vec3::Y_BASIS, Vec3::X_BASIS, Vec3::Z_BASIS, Vec3(0.25f, 0.f, 0.25f));
    DebugAddWorldText("X-Forward", transform, 0.25f, Vec2::ONE, -1.f, Rgba8::RED);

    transform.SetIJKT3D(-Vec3::X_BASIS, -Vec3::Y_BASIS, Vec3::Z_BASIS, Vec3(0.f, 0.25f, 0.5f));
    DebugAddWorldText("Y-Left", transform, 0.25f, Vec2::ZERO, -1.f, Rgba8::GREEN);

    transform.SetIJKT3D(-Vec3::X_BASIS, Vec3::Z_BASIS, Vec3::Y_BASIS, Vec3(0.f, -0.25f, 0.25f));
    DebugAddWorldText("Z-Up", transform, 0.25f, Vec2(1.f, 0.f), -1.f, Rgba8::BLUE);
    // #endif
}

//----------------------------------------------------------------------------------------------------
Match::~Match()
{
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

        float minLength        = FLT_MAX;
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

        float  minLength        = FLT_MAX;
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

bool Match::OnChessMove(EventArgs& args)
{
    String const from       = args.GetValue("from", "DEFAULT");
    String const to         = args.GetValue("to", "DEFAULT");
    String const promotion  = args.GetValue("promoteTo", "DEFAULT");
    bool const   isTeleport = args.GetValue("teleport", false);

    IntVec2 const fromCoords = g_theGame->m_match->m_board->StringToChessCoord(from);
    IntVec2 const toCoords   = g_theGame->m_match->m_board->StringToChessCoord(to);

    g_theGame->m_match->OnChessMove(fromCoords, toCoords, promotion, isTeleport);
    return true;
}

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
