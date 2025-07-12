//----------------------------------------------------------------------------------------------------
// MatchCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Math/IntVec2.hpp"

class Piece;

//----------------------------------------------------------------------------------------------------
enum class eMoveResult : uint8_t
{
    UNKNOWN,
    VALID_MOVE_NORMAL,
    VALID_MOVE_PROMOTION,
    VALID_CASTLE_KINGSIDE,
    VALID_CASTLE_QUEENSIDE,
    VALID_CAPTURE_NORMAL,
    VALID_CAPTURE_ENPASSANT,
    INVALID_MOVE_BAD_LOCATION,
    INVALID_MOVE_NO_PIECE,
    INVALID_MOVE_NOT_YOUR_PIECE,
    INVALID_MOVE_ZERO_DISTANCE,
    INVALID_MOVE_WRONG_MOVE_SHAPE,
    INVALID_MOVE_DESTINATION_BLOCKED,
    INVALID_MOVE_PATH_BLOCKED,
    INVALID_MOVE_ENDS_IN_CHECK,
    INVALID_ENPASSANT_STALE,
    INVALID_CASTLE_KING_HAS_MOVED,
    INVALID_CASTLE_ROOK_HAS_MOVED,
    INVALID_CASTLE_PATH_BLOCKED,
    INVALID_CASTLE_THROUGH_CHECK,
    INVALID_CASTLE_OUT_OF_CHECK
};

struct sMatchRaycastResult
{
    Piece*  m_hitPiece      = nullptr;
    IntVec2 m_currentCoords = IntVec2::ZERO;
    IntVec2 m_targetCoords  = IntVec2::ZERO;
};

struct sPieceMove
{
    Piece const* piece      = nullptr;
    IntVec2      fromCoords = IntVec2::ZERO;
    IntVec2      toCoords   = IntVec2::ZERO;
};

//----------------------------------------------------------------------------------------------------
enum class eChessGameState : uint8_t
{
    WAITING_FOR_CONNECTION,
    WAITING_FOR_OPPONENT,
    PLAYER1_MOVING,
    PLAYER2_MOVING,
    GAME_OVER
};

char const* GetMoveResultString(eMoveResult const& result);
bool        IsMoveValid(eMoveResult const& result);
