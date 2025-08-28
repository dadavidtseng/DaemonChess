//----------------------------------------------------------------------------------------------------
// Game.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Game.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Network/NetworkSubsystem.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Resource/ResourceLoader/ObjModelLoader.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Definition/PieceDefinition.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/PlayerController.hpp"
#include "Game/Gameplay/Match.hpp"

//----------------------------------------------------------------------------------------------------
Game::Game()
{
    g_eventSystem->SubscribeEventCallbackFunction("OnGameStateChanged", OnGameStateChanged);
    g_eventSystem->SubscribeEventCallbackFunction("ChessBegin", OnChessBegin);
    g_eventSystem->SubscribeEventCallbackFunction("ChessPlayerInfo", OnChessPlayerInfo);

    m_gameClock    = new Clock(Clock::GetSystemClock());
    m_screenCamera = new Camera();

    Vec2 const bottomLeft       = Vec2::ZERO;
    Vec2       clientDimensions = Window::s_mainWindow->GetClientDimensions();
    Vec2 const screenTopRight   = Vec2(clientDimensions.x, clientDimensions.y);
    m_screenCamera->SetOrthoGraphicView(bottomLeft, screenTopRight);
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);

    CreateLocalPlayer(0);
    CreateLocalPlayer(1);
    UpdateCurrentControllerId(0);
}

//----------------------------------------------------------------------------------------------------
Game::~Game()
{
    g_eventSystem->UnsubscribeEventCallbackFunction("ChessPlayerInfo", OnChessPlayerInfo);
    g_eventSystem->UnsubscribeEventCallbackFunction("ChessBegin", OnChessBegin);
    g_eventSystem->UnsubscribeEventCallbackFunction("OnGameStateChanged", OnGameStateChanged);
}

//----------------------------------------------------------------------------------------------------
void Game::Update()
{
    // Window::s_mainWindow->UpdateDimension();
    // Window::s_mainWindow->UpdatePosition();
    float const gameDeltaSeconds   = static_cast<float>(m_gameClock->GetDeltaSeconds());
    float const systemDeltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());

    UpdateEntities(gameDeltaSeconds, systemDeltaSeconds);

    UpdateFromInput();
}

//----------------------------------------------------------------------------------------------------
void Game::Render() const
{
    PlayerController const* localPlayer = GetLocalPlayer(m_currentPlayerControllerId);

    //-Start-of-Game-Camera---------------------------------------------------------------------------

    g_renderer->BeginCamera(*localPlayer->GetCamera());

    if (m_gameState == eGameState::MATCH ||
        m_gameState == eGameState::FINISHED)
    {
        RenderEntities();
        g_renderer->RenderEmissive();
    }

    g_renderer->EndCamera(*localPlayer->GetCamera());

    //-End-of-Game-Camera-----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    if (m_gameState == eGameState::MATCH)
    {
        DebugRenderWorld(*localPlayer->GetCamera());
    }
    //------------------------------------------------------------------------------------------------
    //-Start-of-Screen-Camera-------------------------------------------------------------------------

    g_renderer->BeginCamera(*m_screenCamera);

    if (m_gameState == eGameState::ATTRACT)
    {
        RenderAttractMode();
    }
    else if (m_gameState == eGameState::MATCH)
    {
        // g_theRenderer->RenderEmissive();
    }


    g_renderer->EndCamera(*m_screenCamera);

    //-End-of-Screen-Camera---------------------------------------------------------------------------

    DebugRenderScreen(*m_screenCamera);
}

void Game::TogglePlayerControllerId()
{
    if (m_currentPlayerControllerId == 0) m_currentPlayerControllerId = 1;
    else if (m_currentPlayerControllerId == 1) m_currentPlayerControllerId = 0;
}

STATIC bool Game::OnGameStateChanged(EventArgs& args)
{
    String const newGameState = args.GetValue("OnGameStateChanged", "DEFAULT");

    if (newGameState == "ATTRACT")
    {
        PieceDefinition::ClearAllDefs();
        BoardDefinition::ClearAllDefs();
        GAME_SAFE_RELEASE(g_game->m_match);
        g_game->m_currentPlayerControllerId = 0;
    }

    if (newGameState == "MATCH")
    {
        PieceDefinition::InitializeDefs("Data/Definitions/PieceDefinition.xml");
        BoardDefinition::InitializeDefs("Data/Definitions/BoardDefinition.xml");
        g_game->m_match = new Match();
        g_eventSystem->FireEvent("OnMatchInitialized");
    }
    else if (newGameState == "FINISHED")
    {
        int const         id     = g_game->m_currentPlayerControllerId;
        PlayerController* player = g_game->GetLocalPlayer(id);
        player->m_position       = Vec3(9.5f, 4.f, 4.f);
        player->m_orientation    = EulerAngles(180, 45, 0);
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
STATIC bool Game::OnChessBegin(EventArgs& args)
{
    if (g_networkSubsystem->GetConnectionState() == eConnectionState::DISCONNECTED)
    {
        g_devConsole->AddLine(DevConsole::INFO_MAJOR,
                                 Stringf("eConnectionState::DISCONNECTED"));
        return false;
    }
    return true;
}

//----------------------------------------------------------------------------------------------------
STATIC bool Game::OnChessPlayerInfo(EventArgs& args)
{
    if (g_devConsole == nullptr) return false;
    if (g_networkSubsystem == nullptr)
    {
        g_devConsole->AddLine(DevConsole::ERROR, "(OnChessPlayerInfo)NetworkSubsystem is not initialized");

        return false;
    }

    String const name       = args.GetValue("name", "DEFAULT");
    String const type       = args.GetValue("type", "DEFAULT");
    ePlayerType  playerType = ePlayerType::INVALID;
    if (type == "PLAYER") playerType = ePlayerType::PLAYER;
    else if (type == "OPPONENT") playerType = ePlayerType::OPPONENT;
    else if (type == "SPECTATOR") playerType = ePlayerType::SPECTATOR;
    bool const bIsRemote = args.GetValue("remote", false);

    if (playerType == ePlayerType::SPECTATOR)
    {
        static int currentSpectatorID = 2;
        g_game->CreateLocalPlayer(currentSpectatorID);
        g_game->SetLocalPlayerByID(currentSpectatorID, playerType, name);

        currentSpectatorID++;
        // return false;
    }

    if (bIsRemote)
    {
        if (playerType == ePlayerType::SPECTATOR)
        {
            PlayerController* spectator = g_game->GetLocalPlayer(2);
            spectator->SetType(ePlayerType::SPECTATOR);
            spectator->SetName(name);

            g_devConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Spectator joined: %s", name.c_str()));
            sNetworkMessage message;
            message.m_messageType = "RemoteCommand";
            message.m_data = Stringf("Echo text=%s", "SpectatorJoined");
            g_networkSubsystem->SendMessageToAllClients(message);
        }
        else
        {
            // Remote player info - set opponent
            PlayerController* opponent = g_game->GetLocalPlayer(1);
            if (opponent == nullptr)
            {
                // If player 1 doesn't exist, create it first
                opponent = g_game->CreateLocalPlayer(1);
                if (opponent == nullptr)
                {
                    g_devConsole->AddLine(DevConsole::ERROR,
                                             "OnChessPlayerInfo: Failed to create opponent player");
                    return false;
                }
            }

            // Update opponent info
            opponent->SetType(ePlayerType::OPPONENT);
            opponent->SetName(name);

            g_devConsole->AddLine(DevConsole::INFO_MAJOR,
                                     Stringf("Opponent joined: %s", name.c_str()));
            sNetworkMessage message;
            message.m_messageType = "RemoteCommand";
            message.m_data = Stringf("Echo text=%s", "OpponentJoined");
            g_networkSubsystem->SendMessageToAllClients(message);
        }
    }
    else
    {
        // Local player info - set self and send to remote
        PlayerController* localPlayer = g_game->GetLocalPlayer(0);
        if (localPlayer == nullptr)
        {
            // If player 0 doesn't exist, create it first
            localPlayer = g_game->CreateLocalPlayer(0);
            if (localPlayer == nullptr)
            {
                g_devConsole->AddLine(DevConsole::ERROR,
                                         "OnChessPlayerInfo: Failed to create local player");
                return false;
            }
        }

        // Update local player info
        localPlayer->SetType(ePlayerType::PLAYER);
        localPlayer->SetName(name);

        // Check network connection status
        if (g_networkSubsystem->IsConnected())
        {
            // Send local player name to remote
            sNetworkMessage message;
            message.m_messageType = "RemoteCommand";
            message.m_data        = Stringf("ChessPlayerInfo name=%s type=%s remote=true", name.c_str(), type.c_str());

            bool success = false;
            if (g_networkSubsystem->IsClient())
            {
                success = g_networkSubsystem->SendMessageToServer(message);
            }
            else if (g_networkSubsystem->IsServer())
            {
                success = g_networkSubsystem->SendMessageToAllClients(message);
            }

            if (success)
            {
                g_devConsole->AddLine(DevConsole::INFO_MAJOR,
                                         Stringf("Local player set: %s (sent to remote)", name.c_str()));
            }
            else
            {
                g_devConsole->AddLine(DevConsole::WARNING,
                                         Stringf("Local player set: %s (failed to send to remote)", name.c_str()));
            }
        }
        else
        {
            g_devConsole->AddLine(DevConsole::INFO_MAJOR,
                                     Stringf("Local player set: %s (not connected, unable to send to remote)", name.c_str()));
        }
    }

    return true;
}

eGameState Game::GetCurrentGameState() const
{
    return m_gameState;
}

int Game::GetCurrentPlayerControllerId() const
{
    return m_currentPlayerControllerId;
}

void Game::ChangeGameState(eGameState const newGameState)
{
    if (newGameState == m_gameState) return;

    EventArgs args;

    if (newGameState == eGameState::ATTRACT) args.SetValue("OnGameStateChanged", "ATTRACT");
    else if (newGameState == eGameState::MATCH) args.SetValue("OnGameStateChanged", "MATCH");
    else if (newGameState == eGameState::FINISHED) args.SetValue("OnGameStateChanged", "FINISHED");

    m_gameState = newGameState;

    g_eventSystem->FireEvent("OnGameStateChanged", args);
}

bool Game::IsFixedCameraMode() const
{
    return m_isFixedCameraMode;
}

PlayerController* Game::GetCurrentPlayer() const
{
    for (PlayerController* m_localPlayerController : m_localPlayerControllerList)
    {
        if (m_localPlayerController &&
            m_localPlayerController->GetControllerIndex() == m_currentPlayerControllerId)

            return m_localPlayerController;
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------
void Game::UpdateFromInput()
{
    PlayerController const* localPlayer = GetLocalPlayer(m_currentPlayerControllerId);
    UNUSED(localPlayer)
    if (g_input->WasKeyJustPressed(NUMCODE_0))
    {
        Window::s_mainWindow->SetWindowType(eWindowType::FULLSCREEN_CROP);
    }
    if (g_input->WasKeyJustPressed(NUMCODE_1))
    {
        Window::s_mainWindow->SetWindowType(eWindowType::WINDOWED);
    }
    if (m_gameState == eGameState::ATTRACT)
    {
        if (g_input->WasKeyJustPressed(KEYCODE_ESC))
        {
            App::RequestQuit();
        }

        if (g_input->WasKeyJustPressed(KEYCODE_SPACE))
        {
            ChangeGameState(eGameState::MATCH);
        }
    }

    if (m_gameState == eGameState::MATCH ||
        m_gameState == eGameState::FINISHED)
    {
        if (g_input->WasKeyJustPressed(KEYCODE_ESC))
        {
            ChangeGameState(eGameState::ATTRACT);
        }

        if (g_input->WasKeyJustPressed(KEYCODE_P))
        {
            m_gameClock->TogglePause();
        }

        if (g_input->WasKeyJustPressed(KEYCODE_O))
        {
            m_gameClock->StepSingleFrame();
        }

        if (g_input->IsKeyDown(KEYCODE_T))
        {
            m_gameClock->SetTimeScale(0.1f);
        }

        if (g_input->WasKeyJustReleased(KEYCODE_T))
        {
            m_gameClock->SetTimeScale(1.f);
        }

        if (g_input->WasKeyJustPressed(KEYCODE_F6))
        {
            m_currentDebugInt = (m_currentDebugInt + (int)m_currentDebugIntRange.m_max) % (static_cast<int>(m_currentDebugIntRange.GetLength()) + 1);
        }
        if (g_input->WasKeyJustPressed(KEYCODE_F7))
        {
            m_currentDebugInt = (m_currentDebugInt + (int)m_currentDebugIntRange.m_min + 1) % (static_cast<int>(m_currentDebugIntRange.GetLength()) + 1);
        }

        g_renderer->SetPerFrameConstants((float)m_gameClock->GetTotalSeconds(), m_currentDebugInt, 0);

        DebugAddMessage(Stringf("DebugInt=%d|RenderMode=%s", m_currentDebugInt, GetDebugIntString(m_currentDebugInt)), 0.f, Rgba8::YELLOW);

        if (g_input->WasKeyJustPressed(KEYCODE_F4))
        {
            m_isFixedCameraMode = !m_isFixedCameraMode;

            for (int i = 0; i < static_cast<int>(m_localPlayerControllerList.size()); i++)
            {
                m_localPlayerControllerList[i]->SetControllerPosition(g_gameConfigBlackboard.GetValue(Stringf("playerControllerPosition%d", i), Vec3::ZERO));
                m_localPlayerControllerList[i]->SetControllerOrientation(g_gameConfigBlackboard.GetValue(Stringf("playerControllerOrientation%d", i), EulerAngles::ZERO));
                m_localPlayerControllerList[i]->m_worldCamera->SetPositionAndOrientation(m_localPlayerControllerList[i]->m_position, m_localPlayerControllerList[i]->m_orientation);
            }
        }

        // DebugAddMessage(Stringf("Use the DevConsole(~) to enter commands"), 0.f, Rgba8::YELLOW);
        String cameraMode = m_isFixedCameraMode ? "Fixed" : "Free";
        String gameState;
        if (m_currentPlayerControllerId == 0) gameState = "First player's turn.";
        else if (m_currentPlayerControllerId == 1) gameState = "Second player's turn.";
        DebugAddMessage(Stringf("CameraMode=%s|GameState=%s", cameraMode.c_str(), gameState.c_str()), 0.f, Rgba8::YELLOW);
        // DebugAddMessage(Stringf("Player Position: (%.2f, %.2f, %.2f)", localPlayer->m_position.x, localPlayer->m_position.y, localPlayer->m_position.z), 0.f);
    }
}


//----------------------------------------------------------------------------------------------------
void Game::UpdateEntities(float const gameDeltaSeconds, float const systemDeltaSeconds) const
{
    UNUSED(gameDeltaSeconds)
    if (m_match == nullptr) return;
    m_match->Update();
    GetLocalPlayer(m_currentPlayerControllerId)->Update(systemDeltaSeconds);
}

void Game::UpdateCurrentControllerId(int const newID)
{
    m_currentPlayerControllerId = newID;
}

//----------------------------------------------------------------------------------------------------
void Game::RenderAttractMode() const
{
    Vec2 clientDimensions = Window::s_mainWindow->GetClientDimensions();

    VertexList_PCU verts;
    AddVertsForDisc2D(verts, Vec2((float)clientDimensions.x * 0.5f, (float)clientDimensions.y * 0.5f), 300.f, 10.f, Rgba8::YELLOW);
    g_renderer->SetModelConstants();
    g_renderer->SetBlendMode(eBlendMode::OPAQUE);
    g_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
    g_renderer->SetSamplerMode(eSamplerMode::BILINEAR_CLAMP);
    g_renderer->SetDepthMode(eDepthMode::DISABLED);
    g_renderer->BindTexture(nullptr);
    g_renderer->BindShader(g_renderer->CreateOrGetShaderFromFile("Data/Shaders/Default"));
    g_renderer->DrawVertexArray(verts);

    std::vector<std::string> asciiArt = {

        "     '|_.=`   __\\",
        "     `\\_..==`` ",
        "      .'.___.-'.",
        "     /          \\",
        "    ('--......--')",
        "    /'--......--'\\",
        "    `\"--......--\""
    };

    Vec2  basePosition(480.f, 630.f);
    float lineHeight = 40.f;

    for (size_t i = 0; i < asciiArt.size(); ++i)
    {
        Vec2 position = basePosition - Vec2(0.f, i * lineHeight);
        DebugAddScreenText(Stringf(asciiArt[i].c_str()), position, lineHeight, Vec2(0.5f, 0.5f), 0.f);
    }
    DebugAddScreenText(Stringf("Chess Simulator"), Vec2(600.f, 60.f), lineHeight, Vec2(0.5f, 0.5f), 0.f);
    DebugAddScreenText(Stringf("NormalizedMouseUV(%.2f, %.2f)", Window::s_mainWindow->GetNormalizedMouseUV().x, Window::s_mainWindow->GetNormalizedMouseUV().y), m_screenCamera->GetOrthographicBottomLeft(), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    DebugAddScreenText(Stringf("CursorPositionOnScreen(%.1f, %.1f)", Window::s_mainWindow->GetCursorPositionOnScreen().x, Window::s_mainWindow->GetCursorPositionOnScreen().y), m_screenCamera->GetOrthographicBottomLeft() + Vec2(0, 20), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    DebugAddScreenText(Stringf("Window Dimensions(%.1f, %.1f)", Window::s_mainWindow->GetWindowDimensions().x, Window::s_mainWindow->GetWindowDimensions().y), m_screenCamera->GetOrthographicBottomLeft() + Vec2(0, 40), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    DebugAddScreenText(Stringf("Client Dimensions(%.1f, %.1f)", Window::s_mainWindow->GetClientDimensions().x, Window::s_mainWindow->GetClientDimensions().y), m_screenCamera->GetOrthographicBottomLeft() + Vec2(0, 60), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    DebugAddScreenText(Stringf("Viewport Dimensions(%.1f, %.1f)", Window::s_mainWindow->GetViewportDimensions().x, Window::s_mainWindow->GetViewportDimensions().y), m_screenCamera->GetOrthographicBottomLeft() + Vec2(0, 80), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    DebugAddScreenText(Stringf("Screen Dimensions(%.1f, %.1f)", Window::s_mainWindow->GetScreenDimensions().x, Window::s_mainWindow->GetScreenDimensions().y), m_screenCamera->GetOrthographicBottomLeft() + Vec2(0, 100), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
}

//----------------------------------------------------------------------------------------------------
void Game::RenderEntities() const
{
    if (m_match == nullptr) return;
    PlayerController const* localPlayer = GetLocalPlayer(m_currentPlayerControllerId);

    m_match->Render();
    g_renderer->SetModelConstants(localPlayer->GetModelToWorldTransform());
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
    newPlayer->m_worldCamera->SetPositionAndOrientation(newPlayer->m_position, newPlayer->m_orientation);
    m_localPlayerControllerList.push_back(newPlayer);

    return newPlayer;
}

//----------------------------------------------------------------------------------------------------
PlayerController* Game::SetLocalPlayerByID(int const          id,
                                           ePlayerType const& type,
                                           String const&      name)
{
    // if (GetLocalPlayer(id) != nullptr)
    // {
    //     return nullptr;
    // }

    PlayerController* player = m_localPlayerControllerList[id];
    player->SetControllerIndex(id);
    player->SetName(name);
    player->SetType(type);
    player->SetControllerPosition(g_gameConfigBlackboard.GetValue(Stringf("playerControllerPosition%d", id), Vec3::ZERO));
    player->SetControllerOrientation(g_gameConfigBlackboard.GetValue(Stringf("playerControllerOrientation%d", id), EulerAngles::ZERO));
    player->m_worldCamera->SetPositionAndOrientation(player->m_position, player->m_orientation);
    // m_localPlayerControllerList.push_back(player);

    return player;
}

//----------------------------------------------------------------------------------------------------
PlayerController* Game::GetLocalPlayer(int const id) const
{
    for (PlayerController* localPlayerController : m_localPlayerControllerList)
    {
        if (localPlayerController == nullptr) continue;
        if (localPlayerController->GetControllerIndex() != id) continue;

        return localPlayerController;
    }

    return nullptr;
}
