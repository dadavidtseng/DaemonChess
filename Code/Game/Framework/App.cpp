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
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Framework/GameCommon.hpp"
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

    sRendererConfig renderConfig;
    renderConfig.m_window = g_theWindow;
    g_theRenderer         = new Renderer(renderConfig);

    //-End-of-Renderer--------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-DebugRender---------------------------------------------------------------------------

    sDebugRenderConfig debugConfig;
    debugConfig.m_renderer = g_theRenderer;
    debugConfig.m_fontName = "SquirrelFixedFont";

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

    sLightSubsystemConfig constexpr lightConfig;
    g_theLightSubsystem = new LightSubsystem(lightConfig);

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
    DebugRenderSystemStartup(debugConfig);
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
    g_theNetworkSubsystem->BeginFrame();
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

    g_theRenderer->ClearScreen(clearColor);
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

bool Command_NetStartServer(EventArgs& args)
{
    if (!g_theNetworkSubsystem)
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] NetworkSubsystem not initialized");
        return false;
    }

    int const port = args.GetValue("port", 3100);

    if (g_theNetworkSubsystem->StartServer(port))
    {
        g_theDevConsole->AddLine(Rgba8(0, 255, 0),
                                 Stringf("[Network] Server started on port %d", port));
        return true;
    }
    else
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0),
                                 Stringf("[Network] Failed to start server on port %d", port));
        return false;
    }
}

bool Command_NetConnect(EventArgs& args)
{
    if (!g_theNetworkSubsystem)
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] NetworkSubsystem not initialized");
        return false;
    }

    std::string address = args.GetValue("address", "127.0.0.1");  // 預設本機
    int         port    = args.GetValue("port", 3100);

    if (g_theNetworkSubsystem->ConnectToServer(address, port))
    {
        g_theDevConsole->AddLine(Rgba8(0, 255, 0),
                                 Stringf("[Network] Connecting to %s:%d", address.c_str(), port));
        return true;
    }
    else
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0),
                                 Stringf("[Network] Failed to connect to %s:%d", address.c_str(), port));
        return false;
    }
}

bool Command_NetSendTest(EventArgs& args)
{
    if (!g_theNetworkSubsystem)
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] NetworkSubsystem not initialized");
        return false;
    }

    std::string message      = args.GetValue("message", "Hello from console!");
    int         targetClient = args.GetValue("target", -1);  // -1 表示發給所有人

    g_theNetworkSubsystem->SendGameData(message, targetClient);

    if (targetClient == -1)
    {
        g_theDevConsole->AddLine(Rgba8(0, 255, 255),
                                 Stringf("[Network] Sent test message to all: '%s'", message.c_str()));
    }
    else
    {
        g_theDevConsole->AddLine(Rgba8(0, 255, 255),
                                 Stringf("[Network] Sent test message to client %d: '%s'", targetClient, message.c_str()));
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
// Command_NetStopServer
//----------------------------------------------------------------------------------------------------
bool Command_NetStopServer(EventArgs& args)
{
    UNUSED(args)

    if (!g_theNetworkSubsystem)
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] NetworkSubsystem not initialized");
        return false;
    }

    if (!g_theNetworkSubsystem->IsServer())
    {
        g_theDevConsole->AddLine(Rgba8(255, 255, 0), "[Network] Server is not running");
        return false;
    }

    g_theNetworkSubsystem->StopServer();
    g_theDevConsole->AddLine(Rgba8(0, 255, 0), "[Network] Server stopped");
    return true;
}

//----------------------------------------------------------------------------------------------------
// Command_NetDisconnect
//----------------------------------------------------------------------------------------------------
bool Command_NetDisconnect(EventArgs& args)
{
    UNUSED(args)

    if (!g_theNetworkSubsystem)
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] NetworkSubsystem not initialized");
        return false;
    }

    if (!g_theNetworkSubsystem->IsClient())
    {
        g_theDevConsole->AddLine(Rgba8(255, 255, 0), "[Network] Not connected as client");
        return false;
    }

    g_theNetworkSubsystem->DisconnectFromServer();
    g_theDevConsole->AddLine(Rgba8(0, 255, 0), "[Network] Disconnected from server");
    return true;
}

//----------------------------------------------------------------------------------------------------
// Command_NetStatus
//----------------------------------------------------------------------------------------------------
bool Command_NetStatus(EventArgs& args)
{
    UNUSED(args)

    if (!g_theNetworkSubsystem)
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] NetworkSubsystem not initialized");
        return false;
    }

    eNetworkMode     mode  = g_theNetworkSubsystem->GetNetworkMode();
    eConnectionState state = g_theNetworkSubsystem->GetConnectionState();

    std::string modeStr = "NONE";
    if (mode == eNetworkMode::SERVER) modeStr = "SERVER";
    else if (mode == eNetworkMode::CLIENT) modeStr = "CLIENT";

    std::string stateStr = "DISCONNECTED";
    switch (state)
    {
    case eConnectionState::CONNECTING: stateStr = "CONNECTING";
        break;
    case eConnectionState::CONNECTED: stateStr = "CONNECTED";
        break;
    case eConnectionState::DISCONNECTING: stateStr = "DISCONNECTING";
        break;
    case eConnectionState::ERROR_STATE: stateStr = "ERROR";
        break;
    case eConnectionState::DISABLED: stateStr = "DISABLED";
        break;
    }

    g_theDevConsole->AddLine(Rgba8(0, 255, 255),
                             Stringf("[Network] Mode: %s, State: %s", modeStr.c_str(), stateStr.c_str()));

    if (mode == eNetworkMode::SERVER)
    {
        int clientCount = g_theNetworkSubsystem->GetConnectedClientCount();
        g_theDevConsole->AddLine(Rgba8(0, 255, 255),
                                 Stringf("[Network] Connected clients: %d", clientCount));

        std::vector<int> clientIds = g_theNetworkSubsystem->GetConnectedClientIds();
        if (!clientIds.empty())
        {
            std::string clientList = "Client IDs: ";
            for (size_t i = 0; i < clientIds.size(); ++i)
            {
                clientList += std::to_string(clientIds[i]);
                if (i < clientIds.size() - 1) clientList += ", ";
            }
            g_theDevConsole->AddLine(Rgba8(0, 255, 255),
                                     Stringf("[Network] %s", clientList.c_str()));
        }
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
// Command_NetSendChat
//----------------------------------------------------------------------------------------------------
bool Command_NetSendChat(EventArgs& args)
{
    if (!g_theNetworkSubsystem)
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] NetworkSubsystem not initialized");
        return false;
    }

    std::string message      = args.GetValue("message", "Hello from chat!");
    int         targetClient = args.GetValue("target", -1);

    g_theNetworkSubsystem->SendChatMessage(message, targetClient);

    if (targetClient == -1)
    {
        g_theDevConsole->AddLine(Rgba8(0, 255, 255),
                                 Stringf("[Network] Chat sent to all: '%s'", message.c_str()));
    }
    else
    {
        g_theDevConsole->AddLine(Rgba8(0, 255, 255),
                                 Stringf("[Network] Chat sent to client %d: '%s'", targetClient, message.c_str()));
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
// Command_NetSendRaw
//----------------------------------------------------------------------------------------------------
bool Command_NetSendRaw(EventArgs& args)
{
    if (!g_theNetworkSubsystem)
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] NetworkSubsystem not initialized");
        return false;
    }

    std::string data = args.GetValue("data", "test command");

    g_theNetworkSubsystem->SendRawData(data);
    g_theDevConsole->AddLine(Rgba8(0, 255, 255),
                             Stringf("[Network] Sent raw data: '%s'", data.c_str()));

    return true;
}

//----------------------------------------------------------------------------------------------------
// Command_NetHelp
//----------------------------------------------------------------------------------------------------
bool Command_NetHelp(EventArgs& args)
{
    UNUSED(args)

    g_theDevConsole->AddLine(Rgba8(0, 255, 255), "[Network] Available network commands:");
    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "=== Server Commands ===");
    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "  net_start_server [port=3100] - Start server on specified port");
    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "  net_stop_server - Stop running server");

    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "=== Client Commands ===");
    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "  net_connect address=[127.0.0.1] port=[3100] - Connect to server");
    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "  net_disconnect - Disconnect from server");

    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "=== Messaging Commands ===");
    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "  net_send_test message=[Hello] target=[-1] - Send test message");
    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "  net_send_chat message=[Hello] target=[-1] - Send chat message");
    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "  net_send_raw data=[command] - Send raw command data");

    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "=== Utility Commands ===");
    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "  net_status - Show current network status");
    g_theDevConsole->AddLine(Rgba8(255, 255, 255), "  net_help - Show this help");

    g_theDevConsole->AddLine(Rgba8(255, 255, 0), "[Network] TIP: Use target=-1 to send to all clients (server only)");
    g_theDevConsole->AddLine(Rgba8(255, 255, 0), "[Network] Example: net_connect address=192.168.1.100 port=8888");

    return true;
}

//----------------------------------------------------------------------------------------------------
// Command_NetQuickTest
//----------------------------------------------------------------------------------------------------
bool Command_NetQuickTest(EventArgs& args)
{
    UNUSED(args)

    if (!g_theNetworkSubsystem)
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] NetworkSubsystem not initialized");
        return false;
    }

    g_theDevConsole->AddLine(Rgba8(0, 255, 0), "[Network] Starting quick local test...");

    // Start server
    if (g_theNetworkSubsystem->StartServer(3100))
    {
        g_theDevConsole->AddLine(Rgba8(0, 255, 0), "[Network] ✓ Server started on port 3100");
        g_theDevConsole->AddLine(Rgba8(255, 255, 0), "[Network] Now open another instance and run: net_connect");
        g_theDevConsole->AddLine(Rgba8(255, 255, 0), "[Network] Or use net_send_test to simulate messages");
    }
    else
    {
        g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] ✗ Failed to start server");
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
// 當有客戶端連線時觸發
//----------------------------------------------------------------------------------------------------
bool OnClientConnected(EventArgs& args)
{
    int clientId = args.GetValue("clientId", -1);
    if (g_theDevConsole)
    {
        g_theDevConsole->AddLine(Rgba8(0, 255, 0),
                                 Stringf("[Network] *** CLIENT CONNECTED *** Client ID: %d", clientId));
    }
    return true;
}

//----------------------------------------------------------------------------------------------------
// 當有客戶端斷線時觸發
//----------------------------------------------------------------------------------------------------
bool OnClientDisconnected(EventArgs& args)
{
    int clientId = args.GetValue("clientId", -1);
    if (g_theDevConsole)
    {
        g_theDevConsole->AddLine(Rgba8(255, 255, 0),
                                 Stringf("[Network] *** CLIENT DISCONNECTED *** Client ID: %d", clientId));
    }
    return true;
}

//----------------------------------------------------------------------------------------------------
// 當接收到遊戲資料時觸發（這個很重要！）
//----------------------------------------------------------------------------------------------------
bool OnGameDataReceived(EventArgs& args)
{
    std::string data         = args.GetValue("data", "");
    int         fromClientId = args.GetValue("fromClientId", -1);

    // // 清理顯示的資料
    // std::string safeData;
    // for (char c : data)
    // {
    //     if ((c >= 32 && c <= 126) || c == ' ')
    //     {
    //         safeData += c;
    //     }
    //     else
    //     {
    //         safeData += '?'; // 替換不安全的字符
    //     }
    // }

    if (g_theDevConsole)
    {
        if (fromClientId != -1)
        {
            g_theDevConsole->AddLine(Rgba8(255, 255, 255),
                                     Stringf("[Network] *** RECEIVED GAME DATA *** from client %d: '%s'", fromClientId, data.c_str()));
        }
        else
        {
            g_theDevConsole->AddLine(Rgba8(255, 255, 255),
                                     Stringf("[Network] *** RECEIVED GAME DATA *** from server: '%s'", data.c_str()));
        }
    }
    return true;
}

//----------------------------------------------------------------------------------------------------
// 當接收到聊天訊息時觸發
//----------------------------------------------------------------------------------------------------
bool OnChatMessageReceived(EventArgs& args)
{
    std::string message      = args.GetValue("data", "");
    int         fromClientId = args.GetValue("fromClientId", -1);

    if (g_theDevConsole)
    {
        if (fromClientId != -1)
        {
            g_theDevConsole->AddLine(Rgba8(0, 255, 255),
                                     Stringf("[Network] *** RECEIVED CHAT *** from client %d: '%s'", fromClientId, message.c_str()));
        }
        else
        {
            g_theDevConsole->AddLine(Rgba8(0, 255, 255),
                                     Stringf("[Network] *** RECEIVED CHAT *** from server: '%s'", message.c_str()));
        }
    }
    return true;
}

//----------------------------------------------------------------------------------------------------
// 當接收到任何網路訊息時觸發（通用處理器）
//----------------------------------------------------------------------------------------------------
bool OnNetworkMessageReceived(EventArgs& args)
{
    std::string messageType  = args.GetValue("messageType", "");
    std::string data         = args.GetValue("data", "");
    int         fromClientId = args.GetValue("fromClientId", -1);

    if (g_theDevConsole)
    {
        g_theDevConsole->AddLine(Rgba8(200, 200, 200),
                                 Stringf("[Network] Message received - Type: %s, From: %d", messageType.c_str(), fromClientId));
    }
    return true;
}

//----------------------------------------------------------------------------------------------------
// 註冊所有網路事件處理器 - 這是關鍵函數！
//----------------------------------------------------------------------------------------------------
void RegisterNetworkSubsystemEventHandlers()
{
    if (!g_theEventSystem)
    {
        if (g_theDevConsole)
        {
            g_theDevConsole->AddLine(Rgba8(255, 0, 0), "[Network] ERROR: EventSystem not available, cannot register network event handlers!");
        }
        return;
    }

    // 註冊連線事件
    g_theEventSystem->SubscribeEventCallbackFunction("ClientConnected", OnClientConnected);
    g_theEventSystem->SubscribeEventCallbackFunction("ClientDisconnected", OnClientDisconnected);

    // 註冊訊息接收事件（這些是最重要的！）
    g_theEventSystem->SubscribeEventCallbackFunction("GameDataReceived", OnGameDataReceived);
    g_theEventSystem->SubscribeEventCallbackFunction("ChatMessageReceived", OnChatMessageReceived);
    g_theEventSystem->SubscribeEventCallbackFunction("NetworkMessageReceived", OnNetworkMessageReceived);

    if (g_theDevConsole)
    {
        g_theDevConsole->AddLine(Rgba8(0, 255, 255), "[Network] *** EVENT HANDLERS REGISTERED ***");
        g_theDevConsole->AddLine(Rgba8(255, 255, 255), "[Network] - ClientConnected");
        g_theDevConsole->AddLine(Rgba8(255, 255, 255), "[Network] - ClientDisconnected");
        g_theDevConsole->AddLine(Rgba8(255, 255, 255), "[Network] - GameDataReceived");
        g_theDevConsole->AddLine(Rgba8(255, 255, 255), "[Network] - ChatMessageReceived");
        g_theDevConsole->AddLine(Rgba8(255, 255, 255), "[Network] - NetworkMessageReceived");
    }
}
