//----------------------------------------------------------------------------------------------------
// Match.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Board.hpp"
#include "Engine/Core/Clock.hpp"

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

    void    Update(float deltaSeconds) ;
    void UpdateFromInput(float deltaSeconds);

    void    Render() const;
    Camera* m_screenCamera = nullptr;
    Clock*  m_gameClock    = nullptr;


    Board*              m_board            = nullptr;
    std::vector<Piece*> m_pieceList;
    Piece*              m_firstCube  = nullptr;
    Piece*              m_secondCube = nullptr;
    Piece*              m_sphere     = nullptr;
    Piece*              m_grid       = nullptr;
    Clock*              m_clock      = nullptr;
};
