//----------------------------------------------------------------------------------------------------
// Game.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Core/EventSystem.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Camera;
class Clock;
class Match;
class PlayerController;

//----------------------------------------------------------------------------------------------------
enum class eGameState : uint8_t
{
    ATTRACT,
    LOBBY,
    MATCH,
    FINISHED,
    PAUSED
};

//----------------------------------------------------------------------------------------------------
class Game
{
public:
    Game();
    ~Game();

    void Update();
    void Render() const;

    static bool OnGameStateChanged(EventArgs& args);
    static bool OnChessBegin(EventArgs& args);

    eGameState        GetCurrentGameState() const;
    int               GetCurrentPlayerControllerId() const;
    void              TogglePlayerControllerId();
    void              ChangeGameState(eGameState newGameState);
    bool              IsFixedCameraMode() const;
    PlayerController* GetCurrentPlayer() const;
    Match*            m_match = nullptr;

private:
    void              UpdateFromInput();
    void              UpdateEntities(float gameDeltaSeconds, float systemDeltaSeconds) const;
    void              UpdateCurrentControllerId(int newID);
    void              RenderAttractMode() const;
    void              RenderEntities() const;
    PlayerController* CreateLocalPlayer(int id);
    PlayerController* GetLocalPlayer(int id) const;

    Camera*                        m_screenCamera = nullptr;
    AABB2                          m_screenSpace  = AABB2::ZERO_TO_ONE;
    eGameState                     m_gameState    = eGameState::ATTRACT;
    Clock*                         m_gameClock    = nullptr;
    std::vector<PlayerController*> m_localPlayerControllerList;
    int                            m_currentPlayerControllerId = -1;
    bool                           m_isFixedCameraMode         = false;
    int                            m_currentDebugInt           = 0;
    FloatRange                     m_currentDebugIntRange      = FloatRange(0.f, 26.f);

    std::string m_playerName = "Player";  // 預設玩家名稱
    std::string m_opponentName = "";
    bool m_isOpponentConnected = false;
};
