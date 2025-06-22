//----------------------------------------------------------------------------------------------------
// Match.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Game/Definition/PieceDefinition.hpp"
#include "Game/Framework/MatchCommon.hpp"
#include "Game/Gameplay/Board.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Camera;
class Piece;
class PlayerController;

//----------------------------------------------------------------------------------------------------
typedef std::vector<Piece*>     PieceList;
typedef std::vector<sPieceMove> PieceMoveList;

//----------------------------------------------------------------------------------------------------
/// @brief
/// Owned by Game, piece, and board
class Match
{
public:
    Match();
    ~Match();

    void Update();
    void Render() const;

    Board*    m_board = nullptr;
    PieceList m_pieceList;

private:
    void UpdateFromInput(float deltaSeconds);
    void CreateBoard();

    static bool OnChessMove(EventArgs& args);
    static bool OnEnterMatchState(EventArgs& args);
    static bool OnEnterMatchTurn(EventArgs& args);
    static bool OnExitMatchTurn(EventArgs& args);
    static bool OnMatchInitialized(EventArgs& args);

    void OnChessMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promoteTo, bool isTeleport);
    bool ExecuteMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promoteTo, bool isTeleport);

    void ExecuteEnPassantCapture(IntVec2 const& fromCoords, IntVec2 const& toCoords);
    void ExecutePawnPromotion(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promoteTo);
    void ExecuteCastling(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    void ExecuteKingsideCastling(IntVec2 const& fromCoords) const;
    void ExecuteQueensideCastling(IntVec2 const& fromCoords) const;
    void ExecuteCapture(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promoteTo = "");

    void RemovePieceFromPieceList(IntVec2 const& toCoords);

    eMoveResult ValidateChessMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promotionType, bool isTeleport) const;
    eMoveResult ValidatePieceMovement(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promotionType) const;
    eMoveResult ValidatePawnMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promotionType) const;
    eMoveResult ValidateRookMove(int deltaX, int deltaY) const;
    eMoveResult ValidateBishopMove(int absDeltaX, int absDeltaY) const;
    eMoveResult ValidateKnightMove(int absDeltaX, int absDeltaY) const;
    eMoveResult ValidateQueenMove(int deltaX, int deltaY, int absDeltaX, int absDeltaY) const;
    eMoveResult ValidateKingMove(int absDeltaX, int absDeltaY, IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    eMoveResult ValidateCastling(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    eMoveResult DetermineValidMoveType(IntVec2 const& fromCoords, IntVec2 const& toCoords, Piece const* fromPiece) const;

    bool IsKingDistanceValid(IntVec2 const& toCoords) const;
    bool IsPathClear(IntVec2 const& fromCoords, IntVec2 const& toCoords, ePieceType const& pieceType) const;
    bool IsValidEnPassant(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    bool IsValidPromotionType(String const& promoteTo) const;
    
    sPieceMove GetLastPieceMove() const;

    Camera* m_screenCamera = nullptr;
    Clock*  m_gameClock    = nullptr;

    // DEBUG LIGHT
    Vec3  m_sunDirection     = Vec3(2.f, 1.f, -1.f).GetNormalized();
    float m_sunIntensity     = 0.85f;
    float m_ambientIntensity = 0.35f;

    PieceMoveList m_pieceMoveList;
};
