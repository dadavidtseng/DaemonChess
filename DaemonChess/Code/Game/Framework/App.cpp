//----------------------------------------------------------------------------------------------------
// App.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Framework/App.hpp"

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Match.hpp"
#include "Game/Subsystem/Light/LightSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
App*            g_app               = nullptr;       // Created and owned by Main_Windows.cpp
Game*           g_game              = nullptr;       // Created and owned by the App
BitmapFont*     g_bitmapFont     = nullptr;       // Created and owned by the App
LightSubsystem* g_lightSubsystem = nullptr;       // Created and owned by the App
// NetworkSubsystem*      g_theNetworkSubsystem  = nullptr;       // Created and owned by the App
// ResourceSubsystem*     g_resourceSubsystem = nullptr;       // Created and owned by the App

//----------------------------------------------------------------------------------------------------
STATIC bool App::m_isQuitting = false;

App::App()
{
    GEngine::Get().Construct();
}

App::~App()
{
    GEngine::Get().Destruct();
}

//----------------------------------------------------------------------------------------------------
/// @brief
/// Create all engine subsystems in a specific order.
void App::Startup()
{
    LoadGameConfig("Data/GameConfig.xml");

    GEngine::Get().Startup();

    g_eventSystem->SubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);
    g_eventSystem->SubscribeEventCallbackFunction("quit", OnCloseButtonClicked);

    sLightSubsystemConfig constexpr lightSubsystemConfig;
    g_lightSubsystem = new LightSubsystem(lightSubsystemConfig);

    //-End-of-LightSubsystem--------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-NetworkSubsystem----------------------------------------------------------------------

    // sNetworkSubsystemConfig networkSubsystemConfig;
    // networkSubsystemConfig.hostAddressString = "127.0.0.1:3100";
    // networkSubsystemConfig.maxClients        = 4;
    // g_theNetworkSubsystem                    = new NetworkSubsystem(networkSubsystemConfig);

    //-End-of-NetworkSubsystem------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-ResourceSubsystem---------------------------------------------------------------------

    sResourceSubsystemConfig resourceSubsystemConfig;
    resourceSubsystemConfig.m_threadCount = 4;
    resourceSubsystemConfig.m_renderer    = g_renderer;

    g_resourceSubsystem = new ResourceSubsystem(resourceSubsystemConfig);

    //-End-of-ResourceSubsystem-----------------------------------------------------------------------

    g_lightSubsystem->StartUp();
    // g_theNetworkSubsystem->StartUp();
    g_resourceSubsystem->Startup();

    g_game       = new Game();
}

//----------------------------------------------------------------------------------------------------
// All Destroy and ShutDown process should be reverse order of the StartUp
//
void App::Shutdown()
{
    // Destroy all Engine Subsystem
    GAME_SAFE_RELEASE(g_game);


    // g_theResourceSubsystem->Shutdown();
    // g_theNetworkSubsystem->ShutDown();
    g_lightSubsystem->ShutDown();


    GAME_SAFE_RELEASE(g_resourceSubsystem);
    GEngine::Get().Shutdown();
}

//----------------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
//
void App::RunFrame()
{
    BeginFrame();   // Engine pre-frame stuff
    Update();       // Game updates / moves / spawns / hurts / kills stuff
    Render();       // Game draws current state of things
    EndFrame();     // Engine post-frame stuff
}

//----------------------------------------------------------------------------------------------------
void App::RunMainLoop()
{
    // Program main loop; keep running frames until it's time to quit
    while (!m_isQuitting)
    {
        // Sleep(16); // Temporary code to "slow down" our app to ~60Hz until we have proper frame timing in
        RunFrame();
    }
}

//----------------------------------------------------------------------------------------------------
STATIC bool App::OnCloseButtonClicked(EventArgs& args)
{
    UNUSED(args)

    RequestQuit();

    return true;
}

//----------------------------------------------------------------------------------------------------
STATIC void App::RequestQuit()
{
    m_isQuitting = true;
}

//----------------------------------------------------------------------------------------------------
void App::BeginFrame() const
{
    g_eventSystem->BeginFrame();
    g_window->BeginFrame();
    g_renderer->BeginFrame();
    DebugRenderBeginFrame();
    g_devConsole->BeginFrame();
    g_input->BeginFrame();
    g_audio->BeginFrame();
    g_lightSubsystem->BeginFrame();
    // g_theNetworkSubsystem->BeginFrame();
}

//----------------------------------------------------------------------------------------------------
void App::Update()
{
    Clock::TickSystemClock();

    UpdateCursorMode();
    g_game->Update();
    // g_theNetworkSubsystem->Update(Clock::GetSystemClock().GetDeltaSeconds());
}

//----------------------------------------------------------------------------------------------------
// Some simple OpenGL example drawing code.
// This is the graphical equivalent of printing "Hello, world."
//
// Ultimately this function (App::Render) will only call methods on Renderer (like Renderer::DrawVertexArray)
//	to draw things, never calling OpenGL (nor DirectX) functions directly.
//
void App::Render() const
{
    Rgba8 const clearColor = Rgba8::BLACK;

    g_renderer->ClearScreen(clearColor, Rgba8::BLACK);
    g_game->Render();

    AABB2 const box = AABB2(Vec2::ZERO, Vec2(1600.f, 30.f));

    g_devConsole->Render(box);
}

//----------------------------------------------------------------------------------------------------
void App::EndFrame() const
{
    g_eventSystem->EndFrame();
    g_window->EndFrame();
    g_renderer->EndFrame();
    DebugRenderEndFrame();
    g_devConsole->EndFrame();
    g_input->EndFrame();
    g_audio->EndFrame();
    g_lightSubsystem->EndFrame();
    // g_theNetworkSubsystem->EndFrame();
}

//----------------------------------------------------------------------------------------------------
void App::UpdateCursorMode() const
{
    bool const        doesWindowHasFocus   = GetActiveWindow() == g_window->GetWindowHandle();
    bool const        isAttractState       = g_game->GetCurrentGameState() == eGameState::ATTRACT;
    bool const        shouldUsePointerMode = !doesWindowHasFocus || g_devConsole->IsOpen() || isAttractState;
    eCursorMode const mode                 = shouldUsePointerMode ? eCursorMode::POINTER : eCursorMode::FPS;

    g_input->SetCursorMode(mode);
}

//----------------------------------------------------------------------------------------------------
void App::LoadGameConfig(char const* gameConfigXmlFilePath) const
{
    XmlDocument     gameConfigXml;
    XmlResult const result = gameConfigXml.LoadFile(gameConfigXmlFilePath);

    if (result == XmlResult::XML_SUCCESS)
    {
        if (XmlElement const* rootElement = gameConfigXml.RootElement())
        {
            g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);
        }
        else
        {
            DebuggerPrintf("WARNING: game config from file \"%s\" was invalid (missing root element)\n", gameConfigXmlFilePath);
        }
    }
    else
    {
        DebuggerPrintf("WARNING: failed to load game config from file \"%s\"\n", gameConfigXmlFilePath);
    }
}