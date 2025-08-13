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
    static bool OnChessServerInfo(EventArgs& args);
    static bool OnChessListen(EventArgs& args);
    static bool OnChessConnect(EventArgs& args);
    static bool OnRemoteCmd(EventArgs& args);
    static bool OnEcho(EventArgs& args);
    static void RequestQuit();

private:
    void BeginFrame() const;
    void Update();
    void Render() const;
    void EndFrame() const;

    void UpdateCursorMode() const;
    void LoadGameConfig(char const* gameConfigXmlFilePath) const;

    Camera* m_devConsoleCamera = nullptr;
    bool    m_isQuitting       = false;
};
