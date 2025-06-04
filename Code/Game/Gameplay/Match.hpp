//----------------------------------------------------------------------------------------------------
// Match.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Board.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EventSystem.hpp"

class Camera;
class PlayerController;
class Piece;

//----------------------------------------------------------------------------------------------------
/// @brief
/// Owned by Game, owns playerController, piece, and board
class Match
{
public:
    Match();
    ~Match();
    void SpawnProp();

    void Update(float deltaSeconds);
    void UpdateFromInput(float deltaSeconds);

    void        Render() const;
    void        OnChessMove(IntVec2 const& from, IntVec2 const& to);
    static bool OnChessMove(EventArgs& args);
    static bool OnEnterMatchState(EventArgs& args);
    static bool OnEnterMatchTurn(EventArgs& args);
    static bool OnExitMatchTurn(EventArgs& args);

void SwitchPlayerIndex();

    Camera* m_screenCamera = nullptr;
    Clock*  m_gameClock    = nullptr;

    Board*              m_board = nullptr;
    std::vector<Piece*> m_pieceList;
    // Piece*              m_grid       = nullptr;
    Clock*              m_clock      = nullptr;
    int m_currenTurnPlayerIndex = 0;

    // DEBUG LIGHT
    Vec3  m_sunDirection     = Vec3(2.f, 1.f, -1.f).GetNormalized();
    float m_sunIntensity     = 0.85f;
    float m_ambientIntensity = 0.35f;
};
