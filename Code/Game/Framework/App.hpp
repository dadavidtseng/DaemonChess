//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EventSystem.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Camera;

//----------------------------------------------------------------------------------------------------
class App
{
public:
    App()  = default;
    ~App() = default;
    void Startup();
    void Shutdown();
    void RunFrame();

    void RunMainLoop();

    static bool OnCloseButtonClicked(EventArgs& args);
    static void RequestQuit();
    static bool m_isQuitting;

private:
    void BeginFrame() const;
    void Update();
    void Render() const;
    void EndFrame() const;

    void UpdateCursorMode() const;
    void LoadGameConfig(char const* gameConfigXmlFilePath) const;

    Camera* m_devConsoleCamera = nullptr;
};

bool Command_NetStartServer(EventArgs& args);
bool Command_NetConnect(EventArgs& args);
bool Command_NetSendTest(EventArgs& args);
bool Command_NetStopServer(EventArgs& args);
bool Command_NetDisconnect(EventArgs& args);
bool Command_NetStatus(EventArgs& args);
bool Command_NetSendChat(EventArgs& args);
bool Command_NetSendRaw(EventArgs& args);
bool Command_NetHelp(EventArgs& args);
bool Command_NetQuickTest(EventArgs& args);
void RegisterNetworkSubsystemEventHandlers();