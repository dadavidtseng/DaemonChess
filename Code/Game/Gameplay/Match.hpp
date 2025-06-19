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

//----------------------------------------------------------------------------------------------------
class Camera;
class Piece;
class PlayerController;

//----------------------------------------------------------------------------------------------------
typedef std::vector<Piece*> PieceList;

//----------------------------------------------------------------------------------------------------
/// @brief
/// Owned by Game, piece, and board
class Match
{
public:
    Match();
    ~Match();

    void Update();
    void UpdateFromInput(float deltaSeconds);

    void Render() const;

    void CreateBoard();

    static bool OnChessMove(EventArgs& args);
    static bool OnEnterMatchState(EventArgs& args);
    static bool OnEnterMatchTurn(EventArgs& args);
    static bool OnExitMatchTurn(EventArgs& args);
    static bool OnMatchInitialized(EventArgs& args);

    bool IsChessMoveValid(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    void ExecuteMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, std::string const& promotionType);
    void ExecuteEnPassantCapture(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    void ExecutePawnPromotion(IntVec2 const& fromCoords, IntVec2 const& toCoords, std::string const& promotionType) const;
    void ExecuteCastling(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    void HandleCapture(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    void OnChessMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promotionType);
    void UpdatePieceList(IntVec2 const& fromCoords, IntVec2 const& toCoords);

    MoveResult ValidateChessMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, std::string const& promotionType = "") const;
    MoveResult ValidatePieceMovement(IntVec2 const& fromCoords, IntVec2 const& toCoords, std::string const& promotionType) const;
    MoveResult ValidatePawnMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, std::string const& promotionType) const;
    MoveResult ValidateRookMove(int deltaX, int deltaY) const;
    MoveResult ValidateBishopMove(int absDeltaX, int absDeltaY) const;
    MoveResult ValidateKnightMove(int absDeltaX, int absDeltaY) const;
    MoveResult ValidateQueenMove(int deltaX, int deltaY, int absDeltaX, int absDeltaY) const;
    MoveResult ValidateKingMove(int absDeltaX, int absDeltaY, IntVec2 const& fromCoords, IntVec2 const& toCoords) const;

    bool IsKingDistanceValid(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    bool IsPathClear(IntVec2 const& fromCoords, IntVec2 const& toCoords, ePieceType const& pieceType) const;
    bool IsValidEnPassant(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;

    MoveResult ValidateCastling(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    bool       IsValidPromotionType(std::string const& promotionType) const;
    MoveResult DetermineValidMoveType(IntVec2 const& fromCoords, IntVec2 const& toCoords, Piece const* fromPiece, std::string const& promotionType) const;

    Camera* m_screenCamera = nullptr;
    Clock*  m_gameClock    = nullptr;
    Board*  m_board        = nullptr;

    // DEBUG LIGHT
    Vec3  m_sunDirection     = Vec3(2.f, 1.f, -1.f).GetNormalized();
    float m_sunIntensity     = 0.85f;
    float m_ambientIntensity = 0.35f;

    PieceList m_pieceList;
    LastMove  m_lastMove;
};
