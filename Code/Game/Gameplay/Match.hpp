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
typedef std::vector<Piece*> PieceList;
//----------------------------------------------------------------------------------------------------
/// @brief
/// Owned by Game, owns playerController, piece, and board
class Match
{
public:
    Match();
    ~Match();

    void Update(float deltaSeconds);
    void UpdateFromInput(float deltaSeconds);

    void Render() const;

    void CreateBoard();

    bool        IsChessMoveValid(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    void        HandleCapture(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    void        OnChessMove(IntVec2 const& fromCoords, IntVec2 const& toCoords) ;
    void UpdatePieceList(IntVec2 const& fromCoords, IntVec2 const& toCoords);
    static bool OnChessMove(EventArgs& args);
    static bool OnEnterMatchState(EventArgs& args);
    static bool OnEnterMatchTurn(EventArgs& args);
    static bool OnExitMatchTurn(EventArgs& args);
    static bool OnMatchInitialized(EventArgs& args);

    void SwitchPlayerIndex();

    Camera* m_screenCamera          = nullptr;
    Clock*  m_gameClock             = nullptr;
    Board*  m_board                 = nullptr;
    int     m_currenTurnPlayerIndex = 0;

    // DEBUG LIGHT
    Vec3  m_sunDirection     = Vec3(2.f, 1.f, -1.f).GetNormalized();
    float m_sunIntensity     = 0.85f;
    float m_ambientIntensity = 0.35f;

    PieceList                m_pieceList;
};
