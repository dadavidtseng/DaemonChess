//----------------------------------------------------------------------------------------------------
// App.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Framework/App.hpp"

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Network/NetworkSubsystem.hpp"
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
App*                   g_theApp               = nullptr;       // Created and owned by Main_Windows.cpp
AudioSystem*           g_theAudio             = nullptr;       // Created and owned by the App
BitmapFont*            g_theBitmapFont        = nullptr;       // Created and owned by the App
Game*                  g_theGame              = nullptr;       // Created and owned by the App
Renderer*              g_theRenderer          = nullptr;       // Created and owned by the App
RandomNumberGenerator* g_theRNG               = nullptr;       // Created and owned by the App
Window*                g_theWindow            = nullptr;       // Created and owned by the App
LightSubsystem*        g_theLightSubsystem    = nullptr;       // Created and owned by the App
// NetworkSubsystem*      g_theNetworkSubsystem  = nullptr;       // Created and owned by the App
ResourceSubsystem*     g_theResourceSubsystem = nullptr;       // Created and owned by the App

//----------------------------------------------------------------------------------------------------
STATIC bool App::m_isQuitting = false;

//----------------------------------------------------------------------------------------------------
/// @brief
/// Create all engine subsystems in a specific order.
void App::Startup()
{
    LoadGameConfig("Data/GameConfig.xml");

    //-Start-of-EventSystem---------------------------------------------------------------------------

    sEventSystemConfig constexpr eventSystemConfig;
    g_theEventSystem = new EventSystem(eventSystemConfig);
    g_theEventSystem->SubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);
    g_theEventSystem->SubscribeEventCallbackFunction("quit", OnCloseButtonClicked);

    //-End-of-EventSystem-----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-InputSystem---------------------------------------------------------------------------

    sInputSystemConfig constexpr inputConfig;
    g_theInput = new InputSystem(inputConfig);

    //-End-of-InputSystem-----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-Window--------------------------------------------------------------------------------

    sWindowConfig windowConfig;
    windowConfig.m_windowType   = eWindowType::WINDOWED;
    windowConfig.m_aspectRatio  = 2.f;
    windowConfig.m_inputSystem  = g_theInput;
    windowConfig.m_windowTitle  = "ChessSimulator";
    windowConfig.m_iconFilePath = L"C:/p4/Personal/SD/ChessSimulator/Run/Data/Images/Chess.ico";
    g_theWindow                 = new Window(windowConfig);

    //-End-of-Window----------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-Renderer------------------------------------------------------------------------------

    sRendererConfig rendererConfig;
    rendererConfig.m_window = g_theWindow;
    g_theRenderer         = new Renderer(rendererConfig);

    //-End-of-Renderer--------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-DebugRender---------------------------------------------------------------------------

    sDebugRenderConfig debugRenderConfig;
    debugRenderConfig.m_renderer = g_theRenderer;
    debugRenderConfig.m_fontName = "SquirrelFixedFont";

    //-End-of-DebugRender-----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-DevConsole----------------------------------------------------------------------------

    m_devConsoleCamera = new Camera();

    sDevConsoleConfig devConsoleConfig;
    devConsoleConfig.m_defaultRenderer = g_theRenderer;
    devConsoleConfig.m_defaultFontName = "SquirrelFixedFont";
    devConsoleConfig.m_defaultCamera   = m_devConsoleCamera;
    g_theDevConsole                    = new DevConsole(devConsoleConfig);

    g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "Controls");
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "(Mouse) Aim");
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "(W/A)   Move");
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "(S/D)   Strafe");
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "(Q/E)   Roll");
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "(Z/C)   Elevate");
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "(Shift) Sprint");
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "(~)     Toggle Dev Console");
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "(ESC)   Exit Game");
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "(SPACE) Start Game");
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "[Network] Network commands registered. Type 'net_help' for help.");

    //-End-of-DevConsole------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-AudioSystem---------------------------------------------------------------------------

    sAudioSystemConfig constexpr audioSystemConfig;
    g_theAudio = new AudioSystem(audioSystemConfig);

    //-End-of-AudioSystem-----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-LightSubsystem------------------------------------------------------------------------

    sLightSubsystemConfig constexpr lightSubsystemConfig;
    g_theLightSubsystem = new LightSubsystem(lightSubsystemConfig);

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

    g_theResourceSubsystem = new ResourceSubsystem(resourceSubsystemConfig);

    //-End-of-ResourceSubsystem-----------------------------------------------------------------------

    g_theEventSystem->Startup();
    g_theWindow->Startup();
    g_theRenderer->Startup();
    DebugRenderSystemStartup(debugRenderConfig);
    g_theDevConsole->StartUp();
    g_theInput->Startup();
    g_theAudio->Startup();
    g_theLightSubsystem->StartUp();
    // g_theNetworkSubsystem->StartUp();
    g_theResourceSubsystem->Startup();

    g_theBitmapFont = g_theRenderer->CreateOrGetBitmapFontFromFile("Data/Fonts/SquirrelFixedFont"); // DO NOT SPECIFY FILE .EXTENSION!!  (Important later on.)
    g_theRNG        = new RandomNumberGenerator();
    g_theGame       = new Game();
}

//----------------------------------------------------------------------------------------------------
// All Destroy and ShutDown process should be reverse order of the StartUp
//
void App::Shutdown()
{
    // Destroy all Engine Subsystem
    GAME_SAFE_RELEASE(g_theGame);
    GAME_SAFE_RELEASE(g_theRNG);
    GAME_SAFE_RELEASE(g_theBitmapFont);

    // g_theResourceSubsystem->Shutdown();
    // g_theNetworkSubsystem->ShutDown();
    g_theLightSubsystem->ShutDown();
    g_theAudio->Shutdown();
    g_theInput->Shutdown();
    g_theDevConsole->Shutdown();

    GAME_SAFE_RELEASE(m_devConsoleCamera);

    DebugRenderSystemShutdown();
    g_theRenderer->Shutdown();
    g_theWindow->Shutdown();
    g_theEventSystem->Shutdown();

    // GAME_SAFE_RELEASE(g_theResourceSubsystem);
    GAME_SAFE_RELEASE(g_theAudio);
    GAME_SAFE_RELEASE(g_theRenderer);
    GAME_SAFE_RELEASE(g_theWindow);
    GAME_SAFE_RELEASE(g_theInput);
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
    g_theEventSystem->BeginFrame();
    g_theWindow->BeginFrame();
    g_theRenderer->BeginFrame();
    DebugRenderBeginFrame();
    g_theDevConsole->BeginFrame();
    g_theInput->BeginFrame();
    g_theAudio->BeginFrame();
    g_theLightSubsystem->BeginFrame();
    // g_theNetworkSubsystem->BeginFrame();
}

//----------------------------------------------------------------------------------------------------
void App::Update()
{
    Clock::TickSystemClock();

    UpdateCursorMode();
    g_theGame->Update();
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

    g_theRenderer->ClearScreen(clearColor, Rgba8::BLACK);
    g_theGame->Render();

    AABB2 const box = AABB2(Vec2::ZERO, Vec2(1600.f, 30.f));

    g_theDevConsole->Render(box);
}

//----------------------------------------------------------------------------------------------------
void App::EndFrame() const
{
    g_theEventSystem->EndFrame();
    g_theWindow->EndFrame();
    g_theRenderer->EndFrame();
    DebugRenderEndFrame();
    g_theDevConsole->EndFrame();
    g_theInput->EndFrame();
    g_theAudio->EndFrame();
    g_theLightSubsystem->EndFrame();
    // g_theNetworkSubsystem->EndFrame();
}

//----------------------------------------------------------------------------------------------------
void App::UpdateCursorMode() const
{
    bool const        doesWindowHasFocus   = GetActiveWindow() == g_theWindow->GetWindowHandle();
    bool const        isAttractState       = g_theGame->GetCurrentGameState() == eGameState::ATTRACT;
    bool const        shouldUsePointerMode = !doesWindowHasFocus || g_theDevConsole->IsOpen() || isAttractState;
    eCursorMode const mode                 = shouldUsePointerMode ? eCursorMode::POINTER : eCursorMode::FPS;

    g_theInput->SetCursorMode(mode);
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