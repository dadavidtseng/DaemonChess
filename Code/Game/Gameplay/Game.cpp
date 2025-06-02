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
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/PlayerController.hpp"
#include "Game/Gameplay/Match.hpp"

//----------------------------------------------------------------------------------------------------
Game::Game()
{
    m_gameClock = new Clock(Clock::GetSystemClock());
    m_match = new Match();
    CreatePlayerController();
    m_screenCamera = new Camera();
    Vec2 const bottomLeft     = Vec2::ZERO;
    Vec2 const screenTopRight = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);
    m_screenCamera->SetOrthoGraphicView(bottomLeft, screenTopRight);
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
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
    //-Start-of-Game-Camera---------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_playerController->GetCamera());

    if (m_gameState == eGameState::MATCH)
    {
        RenderEntities();
    }

    g_theRenderer->EndCamera(*m_playerController->GetCamera());

    //-End-of-Game-Camera-----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    if (m_gameState == eGameState::MATCH)
    {
        DebugRenderWorld(*m_playerController->GetCamera());
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

eGameState Game::GetCurrentGameState() const
{
    return m_gameState;
}

void Game::ChangeGameState(eGameState const newGameState)
{
    if (newGameState == m_gameState) return;

    m_gameState = newGameState;
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateFromInput()
{
    XboxController const& controller = g_theInput->GetController(0);

    if (m_gameState == eGameState::ATTRACT)
    {
        if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)||
            controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
        {
            App::RequestQuit();
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE)||
            controller.WasButtonJustPressed(XBOX_BUTTON_START))
        {
            m_gameState = eGameState::MATCH;
        }
    }

    if (m_gameState == eGameState::MATCH)
    {
        if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
        {
            m_gameState = eGameState::ATTRACT;
        }


    }

    if (g_theGame->GetCurrentGameState() == eGameState::MATCH)
    {
        if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
        {
            g_theGame->ChangeGameState(eGameState::ATTRACT);
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_P)||
            controller.WasButtonJustPressed(XBOX_BUTTON_B))
        {
            m_gameClock->TogglePause();
        }

        if (g_theInput->WasKeyJustPressed(KEYCODE_O)||
            controller.WasButtonJustPressed(XBOX_BUTTON_Y))
        {
            m_gameClock->StepSingleFrame();
        }

        if (g_theInput->IsKeyDown(KEYCODE_T)||
            controller.WasButtonJustPressed(XBOX_BUTTON_X))
        {
            m_gameClock->SetTimeScale(0.1f);
        }

        if (g_theInput->WasKeyJustReleased(KEYCODE_T)||
            controller.WasButtonJustReleased(XBOX_BUTTON_X))
        {
            m_gameClock->SetTimeScale(1.f);
        }

        if (g_theInput->WasKeyJustPressed(NUMCODE_1))
        {
            Vec3 forward;
            Vec3 right;
            Vec3 up;
            m_playerController->m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, right, up);

            DebugAddWorldLine(m_playerController->m_position, m_playerController->m_position + forward * 20.f, 0.01f, 10.f, Rgba8(255, 255, 0), Rgba8(255, 255, 0), eDebugRenderMode::X_RAY);
        }

        if (g_theInput->IsKeyDown(NUMCODE_2))
        {
            DebugAddWorldPoint(Vec3(m_playerController->m_position.x, m_playerController->m_position.y, 0.f), 0.25f, 60.f, Rgba8(150, 75, 0), Rgba8(150, 75, 0));
        }

        if (g_theInput->WasKeyJustPressed(NUMCODE_3))
        {
            Vec3 forward;
            Vec3 right;
            Vec3 up;
            m_playerController->m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, right, up);

            DebugAddWorldWireSphere(m_playerController->m_position + forward * 2.f, 1.f, 5.f, Rgba8::GREEN, Rgba8::RED);
        }

        if (g_theInput->WasKeyJustPressed(NUMCODE_4))
        {
            DebugAddWorldBasis(m_playerController->GetModelToWorldTransform(), 20.f);
        }

        if (g_theInput->WasKeyJustReleased(NUMCODE_5))
        {
            float const  positionX    = m_playerController->m_position.x;
            float const  positionY    = m_playerController->m_position.y;
            float const  positionZ    = m_playerController->m_position.z;
            float const  orientationX = m_playerController->m_orientation.m_yawDegrees;
            float const  orientationY = m_playerController->m_orientation.m_pitchDegrees;
            float const  orientationZ = m_playerController->m_orientation.m_rollDegrees;
            String const text         = Stringf("Position: (%.2f, %.2f, %.2f)\nOrientation: (%.2f, %.2f, %.2f)", positionX, positionY, positionZ, orientationX, orientationY, orientationZ);

            Vec3 forward;
            Vec3 right;
            Vec3 up;
            m_playerController->m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, right, up);

            DebugAddBillboardText(text, m_playerController->m_position + forward, 0.1f, Vec2::HALF, 10.f, Rgba8::WHITE, Rgba8::RED);
        }

        if (g_theInput->WasKeyJustPressed(NUMCODE_6))
        {
            DebugAddWorldCylinder(m_playerController->m_position, m_playerController->m_position + Vec3::Z_BASIS * 2, 1.f, 10.f, true, Rgba8::WHITE, Rgba8::RED);
        }


        if (g_theInput->WasKeyJustReleased(NUMCODE_7))
        {
            float const orientationX = m_playerController->GetCamera()->GetOrientation().m_yawDegrees;
            float const orientationY = m_playerController->GetCamera()->GetOrientation().m_pitchDegrees;
            float const orientationZ = m_playerController->GetCamera()->GetOrientation().m_rollDegrees;

            DebugAddMessage(Stringf("Camera Orientation: (%.2f, %.2f, %.2f)", orientationX, orientationY, orientationZ), 5.f);
        }

        DebugAddMessage(Stringf("Player Position: (%.2f, %.2f, %.2f)", m_playerController->m_position.x, m_playerController->m_position.y, m_playerController->m_position.z), 0.f);
    }
}


//----------------------------------------------------------------------------------------------------
void Game::UpdateEntities(float const gameDeltaSeconds, float const systemDeltaSeconds) const
{
    m_match->Update(gameDeltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void Game::RenderAttractMode() const
{
    DebugDrawRing(Vec2(800.f, 400.f), 300.f, 10.f, Rgba8::YELLOW);
}

//----------------------------------------------------------------------------------------------------
void Game::RenderEntities() const
{
    g_theRenderer->SetModelConstants(m_playerController->GetModelToWorldTransform());
    m_playerController->Render();
}

void Game::CreatePlayerController()
{
 m_playerController = new PlayerController(this);
}
