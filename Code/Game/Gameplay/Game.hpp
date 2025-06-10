//----------------------------------------------------------------------------------------------------
// Game.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Core/EventSystem.hpp"
#include "Engine/Math/AABB2.hpp"

//----------------------------------------------------------------------------------------------------
class Camera;
class Clock;
class Match;
class PlayerController;

//----------------------------------------------------------------------------------------------------
enum class eGameState : int8_t
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
    void TogglePlayerControllerId();

    static bool OnGameStateChanged(EventArgs& args);

    eGameState GetCurrentGameState() const;
    int        GetCurrentPlayerControllerId() const;
    void       SwitchPlayerControllerId();
    void       ChangeGameState(eGameState newGameState);
    bool       IsFixedCameraMode() const;
    Match*     m_match = nullptr;

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
    bool                           m_isFixedCameraMode         = true;
};
