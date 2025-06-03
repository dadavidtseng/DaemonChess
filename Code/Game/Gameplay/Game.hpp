//----------------------------------------------------------------------------------------------------
// Game.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

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
    MATCH
};

//----------------------------------------------------------------------------------------------------
class Game
{
public:
    Game();
    ~Game();

    void Update();
    void Render() const;

    eGameState GetCurrentGameState() const;
    void       ChangeGameState(eGameState newGameState);
    Match*            m_match            = nullptr;
private:
    void UpdateFromInput();
    void UpdateEntities(float gameDeltaSeconds, float systemDeltaSeconds) const;
    void RenderAttractMode() const;
    void RenderEntities() const;

    void CreatePlayerController();

    Camera*           m_screenCamera     = nullptr;
    AABB2             m_screenSpace      = AABB2::ZERO_TO_ONE;
    eGameState        m_gameState        = eGameState::ATTRACT;
    Clock*            m_gameClock        = nullptr;
    PlayerController* m_playerController = nullptr;
};
