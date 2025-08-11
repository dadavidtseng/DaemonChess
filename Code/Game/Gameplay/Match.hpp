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

/// @brief
/// Owned by Game, piece, and board
class Match
{
public:
    Match();
    ~Match();

    void Update();
    void Render() const;
    void RenderGhostPiece() const;

    Board* m_board = nullptr;
    static bool OnChessMove(EventArgs& args);
private:
    void UpdateFromInput(float deltaSeconds);
    void RenderPlayerBasis() const;
    void CreateScreenCamera();
    void CreateGameClock();
    void CreateBoard();

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
    void SchedulePieceForRemoval(Piece* piece, float delay, ePieceType capturedType);
    void UpdatePendingRemovals(float deltaSeconds);

    eMoveResult ValidateChessMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promotionType, bool isTeleport) const;
    eMoveResult ValidatePieceMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promotionType) const;
    eMoveResult ValidatePawnMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promotionType) const;
    eMoveResult ValidateRookMove(int deltaX, int deltaY) const;
    eMoveResult ValidateBishopMove(int absDeltaX, int absDeltaY) const;
    eMoveResult ValidateKnightMove(int absDeltaX, int absDeltaY) const;
    eMoveResult ValidateQueenMove(int deltaX, int deltaY, int absDeltaX, int absDeltaY) const;
    eMoveResult ValidateKingMove(int absDeltaX, int absDeltaY, IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    eMoveResult ValidateCastling(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    eMoveResult DetermineValidMoveType(IntVec2 const& fromCoords, IntVec2 const& toCoords, Piece const* fromPiece) const;

    /// Helper functions
    bool IsKingDistanceValid(IntVec2 const& toCoords) const;
    bool IsPathClear(IntVec2 const& fromCoords, IntVec2 const& toCoords, ePieceType const& pieceType) const;
    bool IsValidEnPassant(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    bool IsValidPromotionType(String const& promoteTo) const;

    sPieceMove GetLastPieceMove() const;
    Piece*     GetPieceByCoords(IntVec2 const& coords) const;

    Camera*   m_screenCamera = nullptr;
    Clock*    m_gameClock    = nullptr;
    PieceList m_pieceList;

    // DEBUG LIGHT
    Vec3  m_sunDirection     = Vec3(2.f, 1.f, -1.f).GetNormalized();
    float m_sunIntensity     = 0.85f;
    float m_ambientIntensity = 0.35f;

    PieceMoveList m_pieceMoveList;
    Piece*        m_selectedPiece      = nullptr;
    bool          m_showGhostPiece     = false;
    Vec3          m_ghostPiecePosition = Vec3::ZERO;
    Piece*        m_ghostSourcePiece   = nullptr;
    bool          m_isCheatMode        = false;

    // 延遲移除系統
    struct PendingRemoval
    {
        Piece*     piece             = nullptr;
        float      remainingTime     = 0.f;
        ePieceType capturedPieceType = ePieceType::NONE;
    };

    std::vector<PendingRemoval> m_pendingRemovals;
};
