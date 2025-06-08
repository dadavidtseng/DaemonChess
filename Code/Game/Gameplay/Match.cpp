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
#include "Engine/Math/OBB3.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Definition/PieceDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Piece.hpp"

//----------------------------------------------------------------------------------------------------
#if defined ERROR
#undef ERROR
#endif

// #define DEBUG_MODE

//----------------------------------------------------------------------------------------------------
Match::Match()
{
    g_theEventSystem->SubscribeEventCallbackFunction("ChessMove", OnChessMove);
    g_theEventSystem->SubscribeEventCallbackFunction("OnGameStateChanged", OnEnterMatchState);
    g_theEventSystem->SubscribeEventCallbackFunction("OnEnterMatchTurn", OnEnterMatchTurn);
    g_theEventSystem->SubscribeEventCallbackFunction("OnExitMatchTurn", OnExitMatchTurn);
    g_theEventSystem->SubscribeEventCallbackFunction("OnMatchInitialized", OnMatchInitialized);

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
            piece->m_id          = squareInfo.m_playerControllerId;
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
    SafeDeletePointer(m_screenCamera);
    SafeDeletePointer(m_board);

    for (int i = 0; i < static_cast<int>(m_pieceList.size()); ++i)
    {
        SafeDeletePointer(m_pieceList[i]);
    }
    m_pieceList.clear();
}

void Match::Update(float const deltaSeconds)
{
    DebugAddScreenText(Stringf("Time: %.2f\nFPS: %.2f\nScale: %.1f", m_gameClock->GetTotalSeconds(), 1.f / m_gameClock->GetDeltaSeconds(), m_gameClock->GetTimeScale()), m_screenCamera->GetOrthographicTopRight() - Vec2(250.f, 60.f), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);

    UpdateFromInput(deltaSeconds);

    m_board->Update(deltaSeconds);
}

void Match::UpdateFromInput(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}

//----------------------------------------------------------------------------------------------------
void Match::Render() const
{
    g_theRenderer->SetLightConstants(m_sunDirection, m_sunIntensity, m_ambientIntensity);

    m_board->Render();

    for (Piece const* piece : m_pieceList)
    {
        if (piece == nullptr) continue;
        piece->Render();
    }
}

//----------------------------------------------------------------------------------------------------
void Match::CreateBoard()
{
    Texture const* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/WhiteMarble_COLOR.jpg");

    m_board = new Board(this, texture);
}

//----------------------------------------------------------------------------------------------------
bool Match::IsChessMoveValid(IntVec2 const& fromCoords,
                             IntVec2 const& toCoords) const
{
    // 1. If fromCoords nor toCoords is not valid, return false.
    if (!m_board->IsCoordValid(fromCoords) || !m_board->IsCoordValid(toCoords))
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, "[SYSTEM] Invalid board coordinates!");
        return false;
    }

    // 2. If fromCoords if empty, return false.
    Piece const* fromPiece = m_board->GetPieceByCoords(fromCoords);

    if (fromPiece == nullptr)
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("[SYSTEM] from=%s is empty!", m_board->ChessCoordToString(fromCoords).c_str()));
        return false;
    }

    // 3. If fromCoords is not current turn player's piece, return false.
    if (m_board->GetSquareInfoByCoords(fromCoords).m_playerControllerId != g_theGame->GetCurrentPlayerControllerId())
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("[SYSTEM] from=%s is not your piece!", m_board->ChessCoordToString(fromCoords).c_str()));
        return false;
    }

    // 4. If fromCoords equals to toCoords, return false.
    if (fromCoords == toCoords)
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, "Cannot move to the same square.");
        return false;
    }

    // 5. to 格的資訊
    Piece const* toPiece = m_board->GetPieceByCoords(toCoords);
    int const    toOwner = m_board->GetSquareInfoByCoords(toCoords).m_playerControllerId;

    if (toPiece != nullptr)
    {
        if (toOwner == g_theGame->GetCurrentPlayerControllerId())
        {
            g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("[SYSTEM] to=%s is occupied by your own piece!", m_board->ChessCoordToString(toCoords).c_str()));
            return false;
        }
    }

    // This move is valid, return true.
    return true;
}

void Match::HandleCapture(IntVec2 const& fromCoords,
                          IntVec2 const& toCoords) const
{
    Piece const* fromPiece = m_board->GetPieceByCoords(fromCoords);
    Piece const* toPiece   = m_board->GetPieceByCoords(toCoords);

    if (toPiece == nullptr) return;

    // If toCoords's piece is a king, capture it and end the match.
    if (toPiece->m_definition->m_type == ePieceType::KING)
    {
        m_board->CapturePiece(fromPiece->m_coords, toPiece->m_coords);
        g_theDevConsole->AddLine(DevConsole::WARNING, "##################################################");
        g_theDevConsole->AddLine(DevConsole::WARNING, Stringf("[SYSTEM] Player #%d has won the match!", g_theGame->GetCurrentPlayerControllerId()));
        g_theDevConsole->AddLine(DevConsole::WARNING, "##################################################");
        g_theGame->ChangeGameState(eGameState::FINISHED);
    }
    else
    {
        m_board->CapturePiece(fromPiece->m_coords, toPiece->m_coords);
    }
}

void Match::OnChessMove(IntVec2 const& fromCoords,
                        IntVec2 const& toCoords)
{
    if (!IsChessMoveValid(fromCoords, toCoords)) return;

    HandleCapture(fromCoords, toCoords);
    Piece* piece = m_board->GetPieceByCoords(fromCoords);
    piece->UpdatePositionByCoords(toCoords);
    m_board->UpdateBoardSquareInfoList(fromCoords, toCoords);
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("Move Player #%d's %s from %s to %s", g_theGame->GetCurrentPlayerControllerId(), m_board->GetPieceByCoords(fromCoords)->m_definition->m_name.c_str(), m_board->ChessCoordToString(fromCoords).c_str(),
                                                             m_board->ChessCoordToString(toCoords).c_str()));
    g_theEventSystem->FireEvent("OnExitMatchTurn");
}

void Match::UpdatePieceList(IntVec2 const& fromCoords,
                            IntVec2 const& toCoords)
{
    UNUSED(fromCoords);
    for (auto it = m_pieceList.begin(); it != m_pieceList.end();)
    {
        Piece const* piece = *it;

        if (piece->m_coords == toCoords)
        {
            delete piece;                  // 釋放記憶體
            piece = nullptr;
            it    = m_pieceList.erase(it);   // 用 erase 回傳的 iterator（新的位置）
        }
        else
        {
            ++it; // 只有沒刪除時才 ++
        }
    }
}

bool Match::OnChessMove(EventArgs& args)
{
    String from = args.GetValue("from", "DEFAULT");
    String to   = args.GetValue("to", "DEFAULT");

    IntVec2 fromCoords = g_theGame->m_match->m_board->StringToChessCoord(from);
    IntVec2 toCoords   = g_theGame->m_match->m_board->StringToChessCoord(to);

    g_theGame->m_match->OnChessMove(fromCoords, toCoords);
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
