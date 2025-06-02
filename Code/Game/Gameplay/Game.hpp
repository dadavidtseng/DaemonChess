//----------------------------------------------------------------------------------------------------
// Game.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
class Camera;
class Clock;
class Player;
class Piece;

//----------------------------------------------------------------------------------------------------
enum class eGameState
{
    Attract,
    Game
};

//----------------------------------------------------------------------------------------------------
class Game
{
public:
    Game();
    ~Game();

    void Update();
    void Render() const;
    bool IsAttractMode() const;

private:
    void UpdateFromKeyBoard();
    void UpdateFromController();
    void UpdateEntities(float gameDeltaSeconds, float systemDeltaSeconds) const;
    void RenderAttractMode() const;
    void RenderEntities() const;

    void SpawnPlayer();
    void SpawnProp();

    Camera*    m_screenCamera = nullptr;
    Player*    m_player       = nullptr;
    Piece*      m_firstCube    = nullptr;
    Piece*      m_secondCube   = nullptr;
    Piece*      m_sphere       = nullptr;
    Piece*      m_grid         = nullptr;
    Clock*     m_gameClock    = nullptr;
    eGameState m_gameState    = eGameState::Attract;
};
