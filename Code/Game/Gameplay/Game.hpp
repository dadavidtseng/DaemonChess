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
enum class eCameraState : int8_t
{
    FIXED,
    FREE,
    ROTATE
};

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

    static bool OnGameStateChanged(EventArgs& args);

    eGameState GetCurrentGameState() const;
    int        GetCurrentPlayerControllerId() const;
    void       TogglePlayerControllerId();
    void       ChangeGameState(eGameState newGameState);
    bool       IsFixedCameraMode() const;

    Match* m_match = nullptr;

private:
    void              UpdateFromInput();
    void              UpdateEntities(float gameDeltaSeconds, float systemDeltaSeconds) const;
    void              UpdateCurrentControllerId(int newID);
    void              RenderAttractMode() const;
    void              RenderEntities() const;
    PlayerController* CreateLocalPlayer(int id);
    PlayerController* GetLocalPlayer(int id) const;

    void ToggleCameraStateBetweenFreeAndFixed();

    Camera*                        m_screenCamera = nullptr;
    AABB2                          m_screenSpace  = AABB2::ZERO_TO_ONE;
    eGameState                     m_gameState    = eGameState::ATTRACT;
    eCameraState                   m_cameraState  = eCameraState::FIXED;
    Clock*                         m_gameClock    = nullptr;
    std::vector<PlayerController*> m_localPlayerControllerList;
    int                            m_currentPlayerControllerId = -1;
};
