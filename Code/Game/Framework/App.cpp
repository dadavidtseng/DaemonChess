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
NetworkSubsystem*      g_theNetworkSubsystem  = nullptr;       // Created and owned by the App
ResourceSubsystem*     g_theResourceSubsystem = nullptr;       // Created and owned by the App

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
    g_theEventSystem->SubscribeEventCallbackFunction("ChessServerInfo", OnChessServerInfo);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessListen", OnChessListen);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessConnect", OnChessConnect);
    g_theEventSystem->SubscribeEventCallbackFunction("ChessDisconnect", OnChessDisconnect);
    g_theEventSystem->SubscribeEventCallbackFunction("RemoteCmd", OnRemoteCmd);
    g_theEventSystem->SubscribeEventCallbackFunction("Echo", OnEcho);
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
    g_theRenderer           = new Renderer(rendererConfig);

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

    sNetworkSubsystemConfig networkSubsystemConfig;
    networkSubsystemConfig.hostAddressString = "127.0.0.1:3100";
    networkSubsystemConfig.maxClients        = 4;
    g_theNetworkSubsystem                    = new NetworkSubsystem(networkSubsystemConfig);

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
    g_theNetworkSubsystem->StartUp();
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
    g_theNetworkSubsystem->ShutDown();
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
STATIC bool App::OnChessServerInfo(EventArgs& args)
{
    if (g_theNetworkSubsystem == nullptr)
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("(App::OnChessServerInfo)NetworkSubsystem is not initialized"));
        return false;
    }

    String const         newIP   = args.GetValue("ip", "");
    unsigned short const newPort = args.GetValue("port", static_cast<unsigned short>(0));

    // 取得目前的網路狀態
    eNetworkMode     networkMode     = g_theNetworkSubsystem->GetNetworkMode();
    eConnectionState connectionState = g_theNetworkSubsystem->GetConnectionState();
    bool             isConnected     = g_theNetworkSubsystem->IsConnected();

    // 如果目前已連線，拒絕任何變更
    if (isConnected && (!newIP.empty() || newPort != -1))
    {
        g_theDevConsole->AddLine(DevConsole::WARNING, "(App::OnChessServerInfo)Cannot change server info while connected. Disconnect first.");
    }
    else
    {
        // 如果未連線且有提供新的 IP，則更新
        if (!isConnected && !newIP.empty())
        {
            // 這裡需要根據您的系統設計來更新 IP
            // 可能需要存儲在 NetworkSubsystem 或 Game 中
            g_theNetworkSubsystem->SetCurrentIP(newIP);
            g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                     Stringf("Server IP updated to: %s", newIP.c_str()));
        }

        // 如果未連線且有提供新的連接埠，則更新
        if (!isConnected && newPort != -1)
        {
            // 這裡需要根據您的系統設計來更新連接埠
            g_theNetworkSubsystem->SetCurrentPort(newPort);
            g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                     Stringf("Server port updated to: %d", newPort));
        }
    }

    String         currentIP   = g_theNetworkSubsystem->GetCurrentIP();
    unsigned short currentPort = g_theNetworkSubsystem->GetCurrentPort();

    // 如果是伺服器模式
    if (networkMode == eNetworkMode::SERVER)
    {
        int connectedClients = g_theNetworkSubsystem->GetConnectedClientCount();

        g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
                                 Stringf("//////////Chess Client Info//////////"));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("IP: %s", currentIP.c_str()));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Port: %d", currentPort));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Mode: SERVER"));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Status: %s", isConnected ? "LISTENING" : "STOPPED"));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Connected Clients: %d", connectedClients));
    }
    // 如果是客戶端模式
    else if (networkMode == eNetworkMode::CLIENT)
    {
        std::string connectionStatus = "DISCONNECTED";
        switch (connectionState)
        {
        case eConnectionState::CONNECTING:
            connectionStatus = "CONNECTING";
            break;
        case eConnectionState::CONNECTED:
            connectionStatus = "CONNECTED";
            break;
        case eConnectionState::ERROR_STATE:
            connectionStatus = "ERROR";
            break;
        default:
            connectionStatus = "DISCONNECTED";
            break;
        }

        g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
                                 Stringf("//////////Chess Client Info//////////"));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Server IP: %s", currentIP.c_str()));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Server Port: %d", currentPort));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Mode: CLIENT"));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Connection Status: %s", connectionStatus.c_str()));
    }
    // 如果是空閒模式
    else
    {
        g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
                                 Stringf("//////////Chess Client Info//////////"));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("IP: %s", currentIP.c_str()));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Port: %d", currentPort));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Mode: IDLE"));
        g_theDevConsole->AddLine(DevConsole::INFO_MINOR,
                                 Stringf("Status: Not connected"));
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
/// @brief Starts a chess server to listen for incoming client connections.
/// @param args EventArgs containing optional "port" parameter (default: 3100)
/// @return true if server started successfully, false otherwise
STATIC bool App::OnChessListen(EventArgs& args)
{
    if (g_theNetworkSubsystem == nullptr)
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("(App::OnChessListen)NetworkSubsystem is not initialized"));
        return false;
    }

    unsigned short const port    = args.GetValue("port", g_theNetworkSubsystem->GetCurrentPort());
    bool const           success = g_theNetworkSubsystem->StartServer(port);

    if (success)
    {
        g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("(App::OnChessListen)Chess server listening on port %d", port));
    }
    else
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("(App::OnChessListen)Failed to start chess server on port %d", port));
    }

    return success;
}

//----------------------------------------------------------------------------------------------------
/// @brief Connects to a chess server as a client.
/// @param args EventArgs containing optional "ip" (default: "127.0.0.1"), "port" (default: 3100), and "name" (default: "Client") parameters
/// @return true if this event is consumed, false otherwise.
STATIC bool App::OnChessConnect(EventArgs& args)
{
    if (g_theNetworkSubsystem == nullptr)
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("(App::OnChessConnect)NetworkSubsystem is not initialized"));
        return false;
    }

    String const ip      = args.GetValue("ip", g_theNetworkSubsystem->GetCurrentIP());
    int const    port    = args.GetValue("port", g_theNetworkSubsystem->GetCurrentPort());
    bool const   success = g_theNetworkSubsystem->ConnectToServer(ip, port);

    if (success)
    {
        g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("(App::OnChessConnect)Connecting to chess server at %s:%d", ip.c_str(), port));
    }
    else
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("(App::OnChessConnect)Failed to connect to chess server at %s:%d", ip.c_str(), port));
    }

    return success;
}

//----------------------------------------------------------------------------------------------------
STATIC bool App::OnChessDisconnect(EventArgs& args)
{
    if (g_theNetworkSubsystem == nullptr) return false;
    if (g_theDevConsole == nullptr) return false;

    String const reason   = args.GetValue("reason", "Did not provide reason.");
    bool         isRemote = args.GetValue("remote", false);
    g_theNetworkSubsystem->DisconnectFromServer();
    // if (isRemote)
    // {
    //     // 收到來自遠端主機的中斷連線指令
    //     // 協定要求：中斷連線但不要回傳訊息
    //
    //
    //     g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("(App::OnChessDisconnect)(remote)reason=%s", reason.c_str()));
    //     g_theNetworkSubsystem->DisconnectFromServer();
    //
    //     // 只在本地中斷連線，不要回傳指令
    //     // 這裡可以處理應用程式層級的清理工作
    //     // 但實際的網路中斷連線應該由 Match 類別處理
    // }
    // else
    // {
    //     // 本地中斷連線請求
    //     // 協定要求：傳送指令到遠端電腦，然後中斷連線
    //
    //     g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("(App::OnChessDisconnect)(local)reason=%s", reason.c_str()));
    //     // Client 傳送到 Server
    //     sNetworkMessage message;
    //     message.m_messageType = "RemoteCommand";
    //     message.m_data        = "RemoteCmd cmd=ChessDisconnect remote=true";
    //     g_theNetworkSubsystem->SendMessageToServer(message);
    //
    //
    //
    //     // 應用程式層級的處理
    //     // 實際的遠端指令傳送和網路中斷應該由 Match 類別處理
    //
    //     // 注意：這裡我們不直接處理網路操作，而是讓事件繼續傳播到 Match 類別
    // }

    // 返回 false 讓事件繼續傳播到 Match::OnChessDisconnect
    // Match 類別會處理實際的網路操作（傳送指令和中斷連線）
    return false;
}

//----------------------------------------------------------------------------------------------------
/// @brief Sends a DevConsole command to the remote computer for execution.
/// @param args EventArgs containing "cmd" parameter with the command name, and optional key-value pairs for command parameters
/// @return true if command sent successfully, false otherwise
STATIC bool App::OnRemoteCmd(EventArgs& args)
{
    if (g_theNetworkSubsystem == nullptr)
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("(App::OnRemoteCmd)NetworkSubsystem is not initialized"));
        return false;
    }

    // 取得 cmd 參數（實際要執行的命令名稱）
    String const cmd = args.GetValue("cmd", "");
    if (cmd.empty())
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, "(App::OnRemoteCmd)RemoteCmd requires cmd=<commandName>");
        return false;
    }

    // 建立要傳送的命令字串
    std::string remoteCommandString = cmd;

    // 將所有其他參數（除了 cmd）加入命令字串
    std::map<String, String> allArgs = args.GetAllKeyValuePairs();
    for (auto const& [key, value] : allArgs)
    {
        // 跳過 "cmd" 參數，因為已經處理過了
        if (key != "cmd" && !value.empty())
        {
            remoteCommandString += " " + key + "=" + value;
        }
    }

    // 透過網路傳送命令字串
    if (g_theNetworkSubsystem->IsClient())
    {
        // Client 傳送到 Server
        sNetworkMessage message;
        message.m_messageType = "RemoteCommand";
        message.m_data        = remoteCommandString;

        if (g_theNetworkSubsystem->SendMessageToServer(message))
        {
            g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
                                     Stringf("Sent to server: %s", remoteCommandString.c_str()));
            return true;
        }
    }
    else if (g_theNetworkSubsystem->IsServer())
    {
        // Server 傳送到所有 Clients
        sNetworkMessage message;
        message.m_messageType = "RemoteCommand";
        message.m_data        = remoteCommandString;

        if (g_theNetworkSubsystem->SendMessageToAllClients(message))
        {
            g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
                                     Stringf("Sent to all clients: %s", remoteCommandString.c_str()));
            return true;
        }
    }

    g_theDevConsole->AddLine(DevConsole::ERROR, "Failed to send remote command");
    return false;
}

//----------------------------------------------------------------------------------------------------
/// @brief Echo command for testing network communication.
/// @param args EventArgs containing "text" parameter to echo, optional "remote" flag
/// @return Always returns true
STATIC bool App::OnEcho(EventArgs& args)
{
    String const text     = args.GetValue("text", "DEFAULT");
    bool const   isRemote = args.GetValue("remote", false);

    g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Echo(%s): %s", isRemote ? "remote" : "local", text.c_str()));

    return true;
}

//----------------------------------------------------------------------------------------------------
STATIC void App::RequestQuit()
{
    g_theApp->m_isQuitting = true;
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
    g_theNetworkSubsystem->BeginFrame();
}

//----------------------------------------------------------------------------------------------------
void App::Update()
{
    Clock::TickSystemClock();

    UpdateCursorMode();
    g_theGame->Update();
    g_theNetworkSubsystem->Update();
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
    g_theNetworkSubsystem->EndFrame();
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
