//----------------------------------------------------------------------------------------------------
// Match.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Match.hpp"

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Input/XboxController.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Definition/PieceDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Piece.hpp"

Match::Match()
{
    g_theEventSystem->SubscribeEventCallbackFunction("ChessMove", OnChessMove);
    g_theEventSystem->SubscribeEventCallbackFunction("OnGameStateChanged", OnEnterMatchState);
    g_theEventSystem->SubscribeEventCallbackFunction("OnEnterMatchTurn", OnEnterMatchTurn);
    g_theEventSystem->SubscribeEventCallbackFunction("OnExitMatchTurn", OnExitMatchTurn);
    m_screenCamera = new Camera();

    Vec2 const bottomLeft     = Vec2::ZERO;
    Vec2 const screenTopRight = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);

    m_screenCamera->SetOrthoGraphicView(bottomLeft, screenTopRight);
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    m_gameClock = new Clock(Clock::GetSystemClock());


    DebugAddWorldBasis(Mat44(), -1.f);

    Mat44 transform;

    transform.SetIJKT3D(-Vec3::Y_BASIS, Vec3::X_BASIS, Vec3::Z_BASIS, Vec3(0.25f, 0.f, 0.25f));
    DebugAddWorldText("X-Forward", transform, 0.25f, Vec2::ONE, -1.f, Rgba8::RED);

    transform.SetIJKT3D(-Vec3::X_BASIS, -Vec3::Y_BASIS, Vec3::Z_BASIS, Vec3(0.f, 0.25f, 0.5f));
    DebugAddWorldText("Y-Left", transform, 0.25f, Vec2::ZERO, -1.f, Rgba8::GREEN);

    transform.SetIJKT3D(-Vec3::X_BASIS, Vec3::Z_BASIS, Vec3::Y_BASIS, Vec3(0.f, -0.25f, 0.25f));
    DebugAddWorldText("Z-Up", transform, 0.25f, Vec2(1.f, 0.f), -1.f, Rgba8::BLUE);

    SpawnProp();

    // m_grid->m_position = Vec3::ZERO;
    m_clock            = new Clock(Clock::GetSystemClock());
}

Match::~Match()
{
    // delete m_grid;
    // m_grid = nullptr;

    delete m_gameClock;
    m_gameClock = nullptr;

    delete m_screenCamera;
    m_screenCamera = nullptr;
}


//----------------------------------------------------------------------------------------------------
void Match::SpawnProp()
{
    Texture const* texture  = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestUV.png");
    Texture const* texture2 = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");

    // m_grid  = new Piece(this);
    m_board = new Board(this);
    m_pieceList = m_board->m_pieceList;
    // m_grid->InitializeLocalVertsForGrid();
}

void Match::Update(float const deltaSeconds)
{
    // m_grid->Update(deltaSeconds);

    DebugAddScreenText(Stringf("Time: %.2f\nFPS: %.2f\nScale: %.1f", m_gameClock->GetTotalSeconds(), 1.f / m_gameClock->GetDeltaSeconds(), m_gameClock->GetTimeScale()), m_screenCamera->GetOrthographicTopRight() - Vec2(250.f, 60.f), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    UpdateFromInput(deltaSeconds);

    m_board->Update(deltaSeconds);
}

// TODO: controller keybinding
void Match::UpdateFromInput(float deltaSeconds)
{
    XboxController const& controller = g_theInput->GetController(0);

    if (g_theGame->GetCurrentGameState() == eGameState::MATCH)
    {
        if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
        {
            g_theGame->ChangeGameState(eGameState::ATTRACT);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_P) ||
            controller.WasButtonJustPressed(XBOX_BUTTON_B))
        {
            m_gameClock->TogglePause();
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_O) ||
            controller.WasButtonJustPressed(XBOX_BUTTON_Y))
        {
            m_gameClock->StepSingleFrame();
        }

        if (g_theInput->IsKeyDown(KEYCODE_T) ||
            controller.WasButtonJustPressed(XBOX_BUTTON_X))
        {
            m_gameClock->SetTimeScale(0.1f);
        }

        if (g_theInput->WasKeyJustReleased(KEYCODE_T) ||
            controller.WasButtonJustReleased(XBOX_BUTTON_X))
        {
            m_gameClock->SetTimeScale(1.f);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_I))
        {
            DebugAddMessage(Stringf("Sun Direction: (%.2f, %.2f, %.2f)", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z), 5.f);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
        {
            m_sunDirection.x -= 1.f;
            DebugAddMessage(Stringf("Sun Direction: (%.2f, %.2f, %.2f)", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z), 5.f);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
        {
            m_sunDirection.x += 1.f;
            DebugAddMessage(Stringf("Sun Direction: (%.2f, %.2f, %.2f)", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z), 5.f);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_F4))
        {
            m_sunDirection.y -= 1.f;
            DebugAddMessage(Stringf("Sun Direction: (%.2f, %.2f, %.2f)", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z), 5.f);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_F5))
        {
            m_sunDirection.y += 1.f;
            DebugAddMessage(Stringf("Sun Direction: (%.2f, %.2f, %.2f)", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z), 5.f);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
        {
            m_sunIntensity -= 0.05f;
            m_sunIntensity = GetClampedZeroToOne(m_sunIntensity);
            DebugAddMessage(Stringf("Sun Intensity: (%.2f)", m_sunIntensity), 5.f);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
        {
            m_sunIntensity += 0.05f;
            m_sunIntensity = GetClampedZeroToOne(m_sunIntensity);
            DebugAddMessage(Stringf("Sun Intensity: (%.2f)", m_sunIntensity), 5.f);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
        {
            m_ambientIntensity -= 0.05f;
            m_ambientIntensity = GetClampedZeroToOne(m_ambientIntensity);
            DebugAddMessage(Stringf("Ambient Intensity: (%.2f)", m_ambientIntensity), 5.f);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_F9))
        {
            m_ambientIntensity += 0.05f;
            m_ambientIntensity = GetClampedZeroToOne(m_ambientIntensity);
            DebugAddMessage(Stringf("Ambient Intensity: (%.2f)", m_ambientIntensity), 5.f);
        }
    }
}


void Match::Render() const
{
    g_theRenderer->SetLightConstants(m_sunDirection, m_sunIntensity, m_ambientIntensity);
    // m_grid->Render();

    m_board->Render();

    for (Piece* piece:m_pieceList)
    {
        if (piece==nullptr) continue;
        piece->Render();
    }
}

void Match::OnChessMove(IntVec2 const& from,
                        IntVec2 const& to)
{
    DebuggerPrintf(Stringf("OnChessMove from=%d to=%d\n", from, to).c_str());
    Piece* piece =m_board->GetPieceByCoords(from);
    piece->UpdatePositionByCoords(to);
    g_theEventSystem->FireEvent("OnEnterMatchTurn");
}

bool Match::OnChessMove(EventArgs& args)
{
    String from = args.GetValue("from", "DEFAULT");
    String to   = args.GetValue("to",  "DEFAULT");

IntVec2 fromCoords = g_theGame->m_match->m_board->StringToChessCoord(from);
    IntVec2 toCoords = g_theGame->m_match->m_board->StringToChessCoord( to);

    g_theGame->m_match->OnChessMove(fromCoords, toCoords);
    return true;
}

bool Match::OnEnterMatchState(EventArgs& args)
{
    OnEnterMatchTurn(args);
    return true;
}

bool Match::OnEnterMatchTurn(EventArgs& args)
{
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("=================================================="));
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("Player #%d -- it's your turn!", g_theGame->m_match->m_currenTurnPlayerIndex));

    int const currentTurnPlayerIndex = g_theGame->m_match->m_currenTurnPlayerIndex;

    if (currentTurnPlayerIndex == 0) g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Game state is: First Player's Turn"));
    else if (currentTurnPlayerIndex == 1) g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Game state is: Second Player's Turn"));

    g_theEventSystem->FireEvent("OnExitMatchTurn");
    return true;
}

bool Match::OnExitMatchTurn(EventArgs& args)
{
    g_theGame->m_match->SwitchPlayerIndex();
    // g_theEventSystem->FireEvent("OnEnterMatchTurn");
    return true;
}

void Match::SwitchPlayerIndex()
{
    if (m_currenTurnPlayerIndex == 0) m_currenTurnPlayerIndex = 1;
    else if (m_currenTurnPlayerIndex == 1) m_currenTurnPlayerIndex = 0;
}
