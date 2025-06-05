//----------------------------------------------------------------------------------------------------
// Game.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Game.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Definition/PieceDefinition.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/PlayerController.hpp"
#include "Game/Gameplay/Match.hpp"

//----------------------------------------------------------------------------------------------------
Game::Game()
{
    g_theEventSystem->SubscribeEventCallbackFunction("OnGameStateChanged", OnGameStateChanged);
    m_gameClock               = new Clock(Clock::GetSystemClock());
    m_screenCamera            = new Camera();
    Vec2 const bottomLeft     = Vec2::ZERO;
    Vec2 const screenTopRight = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);
    m_screenCamera->SetOrthoGraphicView(bottomLeft, screenTopRight);
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    CreateLocalPlayer(0);
    CreateLocalPlayer(1);
    UpdateCurrentControllerId(0);
    PieceDefinition::InitializeDefs("Data/Definitions/PieceDefinition.xml");
    BoardDefinition::InitializeDefs("Data/Definitions/BoardDefinition.xml");
}

//----------------------------------------------------------------------------------------------------
Game::~Game()
{
}

//----------------------------------------------------------------------------------------------------
void Game::Update()
{
    float const gameDeltaSeconds   = static_cast<float>(m_gameClock->GetDeltaSeconds());
    float const systemDeltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());

    // #TODO: Select keyboard or controller
    UpdateEntities(gameDeltaSeconds, systemDeltaSeconds);

    UpdateFromInput();
}

//----------------------------------------------------------------------------------------------------
void Game::Render() const
{
    PlayerController const* localPlayer = GetLocalPlayer(m_currentControllerId);

    //-Start-of-Game-Camera---------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*localPlayer->GetCamera());

    if (m_gameState == eGameState::MATCH)
    {
        RenderEntities();
    }

    g_theRenderer->EndCamera(*localPlayer->GetCamera());

    //-End-of-Game-Camera-----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    if (m_gameState == eGameState::MATCH)
    {
        DebugRenderWorld(*localPlayer->GetCamera());
    }
    //------------------------------------------------------------------------------------------------
    //-Start-of-Screen-Camera-------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_screenCamera);

    if (m_gameState == eGameState::ATTRACT)
    {
        RenderAttractMode();
    }

    g_theRenderer->EndCamera(*m_screenCamera);

    //-End-of-Screen-Camera---------------------------------------------------------------------------
    if (m_gameState == eGameState::MATCH)
    {
        DebugRenderScreen(*m_screenCamera);
    }
}

bool Game::OnGameStateChanged(EventArgs& args)
{
    if (args.GetValue("OnGameStateChanged", "DEFAULT") == "MATCH")
    {
        g_theGame->m_match = new Match();
        g_theEventSystem->FireEvent("OnMatchInitialized");
    }

    return true;
}

eGameState Game::GetCurrentGameState() const
{
    return m_gameState;
}

void Game::ChangeGameState(eGameState const newGameState)
{
    if (newGameState == m_gameState) return;

    EventArgs args;

    if (newGameState == eGameState::ATTRACT) args.SetValue("OnGameStateChanged", "ATTRACT");
    else if (newGameState == eGameState::MATCH) args.SetValue("OnGameStateChanged", "MATCH");

    m_gameState = newGameState;

    g_theEventSystem->FireEvent("OnGameStateChanged", args);
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateFromInput()
{
    PlayerController const* localPlayer = GetLocalPlayer(m_currentControllerId);

    if (m_gameState == eGameState::ATTRACT)
    {
        if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
        {
            App::RequestQuit();
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
        {
            ChangeGameState(eGameState::MATCH);
        }
    }

    if (m_gameState == eGameState::MATCH)
    {
        if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
        {
            m_gameState = eGameState::ATTRACT;
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_P))
        {
            m_gameClock->TogglePause();
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_O))
        {
            m_gameClock->StepSingleFrame();
        }

        if (g_theInput->IsKeyDown(KEYCODE_T))
        {
            m_gameClock->SetTimeScale(0.1f);
        }

        if (g_theInput->WasKeyJustReleased(KEYCODE_T))
        {
            m_gameClock->SetTimeScale(1.f);
        }

        if (g_theInput->WasKeyJustPressed(NUMCODE_1))
        {
            Vec3 forward;
            Vec3 right;
            Vec3 up;
            localPlayer->m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, right, up);

            UpdateCurrentControllerId(0);
            DebugAddWorldLine(localPlayer->m_position, localPlayer->m_position + forward * 20.f, 0.01f, 10.f, Rgba8(255, 255, 0), Rgba8(255, 255, 0), eDebugRenderMode::X_RAY);
        }

        if (g_theInput->IsKeyDown(NUMCODE_2))
        {
            UpdateCurrentControllerId(1);
            DebugAddWorldPoint(Vec3(localPlayer->m_position.x, localPlayer->m_position.y, 0.f), 0.25f, 60.f, Rgba8(150, 75, 0), Rgba8(150, 75, 0));
        }

        if (g_theInput->WasKeyJustPressed(NUMCODE_3))
        {
            Vec3 forward;
            Vec3 right;
            Vec3 up;
            localPlayer->m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, right, up);

            DebugAddWorldWireSphere(localPlayer->m_position + forward * 2.f, 1.f, 5.f, Rgba8::GREEN, Rgba8::RED);
        }

        if (g_theInput->WasKeyJustPressed(NUMCODE_4))
        {
            DebugAddWorldBasis(localPlayer->GetModelToWorldTransform(), 20.f);
        }

        if (g_theInput->WasKeyJustReleased(NUMCODE_5))
        {
            float const  positionX    = localPlayer->m_position.x;
            float const  positionY    = localPlayer->m_position.y;
            float const  positionZ    = localPlayer->m_position.z;
            float const  orientationX = localPlayer->m_orientation.m_yawDegrees;
            float const  orientationY = localPlayer->m_orientation.m_pitchDegrees;
            float const  orientationZ = localPlayer->m_orientation.m_rollDegrees;
            String const text         = Stringf("Position: (%.2f, %.2f, %.2f)\nOrientation: (%.2f, %.2f, %.2f)", positionX, positionY, positionZ, orientationX, orientationY, orientationZ);

            Vec3 forward;
            Vec3 right;
            Vec3 up;
            localPlayer->m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, right, up);

            DebugAddBillboardText(text, localPlayer->m_position + forward, 0.1f, Vec2::HALF, 10.f, Rgba8::WHITE, Rgba8::RED);
        }

        if (g_theInput->WasKeyJustPressed(NUMCODE_6))
        {
            DebugAddWorldCylinder(localPlayer->m_position, localPlayer->m_position + Vec3::Z_BASIS * 2, 1.f, 10.f, true, Rgba8::WHITE, Rgba8::RED);
        }


        if (g_theInput->WasKeyJustReleased(NUMCODE_7))
        {
            float const orientationX = localPlayer->GetCamera()->GetOrientation().m_yawDegrees;
            float const orientationY = localPlayer->GetCamera()->GetOrientation().m_pitchDegrees;
            float const orientationZ = localPlayer->GetCamera()->GetOrientation().m_rollDegrees;

            DebugAddMessage(Stringf("Camera Orientation: (%.2f, %.2f, %.2f)", orientationX, orientationY, orientationZ), 5.f);
        }

        DebugAddMessage(Stringf("Player Position: (%.2f, %.2f, %.2f)", localPlayer->m_position.x, localPlayer->m_position.y, localPlayer->m_position.z), 0.f);
    }
}


//----------------------------------------------------------------------------------------------------
void Game::UpdateEntities(float const gameDeltaSeconds, float const systemDeltaSeconds) const
{
    if (m_match == nullptr) return;
    m_match->Update(gameDeltaSeconds);
    GetLocalPlayer(m_currentControllerId)->Update(systemDeltaSeconds);
}

void Game::UpdateCurrentControllerId(int const newID)
{
    m_currentControllerId = newID;
}

//----------------------------------------------------------------------------------------------------
void Game::RenderAttractMode() const
{
    DebugDrawRing(Vec2(800.f, 400.f), 300.f, 10.f, Rgba8::YELLOW);
}

//----------------------------------------------------------------------------------------------------
void Game::RenderEntities() const
{
    if (m_match == nullptr) return;
    PlayerController const* localPlayer = GetLocalPlayer(m_currentControllerId);

    m_match->Render();
    g_theRenderer->SetModelConstants(localPlayer->GetModelToWorldTransform());
    localPlayer->Render();
}

PlayerController* Game::CreateLocalPlayer(int const id)
{
    PlayerController* newPlayer = new PlayerController(nullptr);

    for (PlayerController const* controller : m_localPlayerControllerList)
    {
        if (controller && controller->GetControllerIndex() == id)
        {
            return nullptr;
        }
    }

    newPlayer->SetControllerIndex(id);
    newPlayer->SetControllerPosition(g_gameConfigBlackboard.GetValue(Stringf("playerControllerPosition%d", id), Vec3::ZERO));
    newPlayer->SetControllerOrientation(g_gameConfigBlackboard.GetValue(Stringf("playerControllerOrientation%d", id), EulerAngles::ZERO));

    m_localPlayerControllerList.push_back(newPlayer);

    return newPlayer;
}

//----------------------------------------------------------------------------------------------------
PlayerController* Game::GetLocalPlayer(int const id) const
{
    for (PlayerController* m_localPlayerController : m_localPlayerControllerList)
    {
        if (m_localPlayerController &&
            m_localPlayerController->GetControllerIndex() == id)

            return m_localPlayerController;
    }

    return nullptr;
}
