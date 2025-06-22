//----------------------------------------------------------------------------------------------------
// MatchCommon.cpp
//----------------------------------------------------------------------------------------------------

#include "Game/Framework/MatchCommon.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
char const* GetMoveResultString(eMoveResult const& result)
{
    switch (result)
    {
    case eMoveResult::UNKNOWN: return "Unknown ChessMoveResult!";
    case eMoveResult::VALID_MOVE_NORMAL: return "Valid move";
    case eMoveResult::VALID_MOVE_PROMOTION: return "Valid move, resulting in pawn promotion";
    case eMoveResult::VALID_CASTLE_KINGSIDE: return "Valid move, castling kingside";
    case eMoveResult::VALID_CASTLE_QUEENSIDE: return "Valid move, castling queenside";
    case eMoveResult::VALID_CAPTURE_NORMAL: return "Valid move, capturing enemy piece";
    case eMoveResult::VALID_CAPTURE_ENPASSANT: return "Valid move, capturing enemy pawn en passant";
    case eMoveResult::INVALID_MOVE_BAD_LOCATION: return "Invalid move; invalid board location given";
    case eMoveResult::INVALID_MOVE_NO_PIECE: return "Invalid move; no piece at location given";
    case eMoveResult::INVALID_MOVE_NOT_YOUR_PIECE: return "Invalid move; can't move opponent's piece";
    case eMoveResult::INVALID_MOVE_ZERO_DISTANCE: return "Invalid move; didn't go anywhere";
    case eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE: return "Invalid move; piece cannot move in that way";
    case eMoveResult::INVALID_MOVE_DESTINATION_BLOCKED: return "Invalid move; destination is blocked by your piece";
    case eMoveResult::INVALID_MOVE_PATH_BLOCKED: return "Invalid move; path is blocked by your piece";
    case eMoveResult::INVALID_MOVE_ENDS_IN_CHECK: return "Invalid move; can't leave yourself in check";
    case eMoveResult::INVALID_ENPASSANT_STALE: return "Invalid move; en passant must immediately follow a pawn double-move";
    case eMoveResult::INVALID_CASTLE_KING_HAS_MOVED: return "Invalid castle; king has moved previously";
    case eMoveResult::INVALID_CASTLE_ROOK_HAS_MOVED: return "Invalid castle; that rook has moved previously";
    case eMoveResult::INVALID_CASTLE_PATH_BLOCKED: return "Invalid castle; pieces in-between king and rook";
    case eMoveResult::INVALID_CASTLE_THROUGH_CHECK: return "Invalid castle; king can't move through check";
    case eMoveResult::INVALID_CASTLE_OUT_OF_CHECK: return "Invalid castle; king can't castle out of check";
    default: ERROR_AND_DIE(Stringf("Unhandled MoveResult enum value #%d", result))
    }
}

bool IsMoveValid(eMoveResult const& result)
{
    switch (result)
    {
    case eMoveResult::VALID_MOVE_NORMAL:
    case eMoveResult::VALID_MOVE_PROMOTION:
    case eMoveResult::VALID_CASTLE_KINGSIDE:
    case eMoveResult::VALID_CASTLE_QUEENSIDE:
    case eMoveResult::VALID_CAPTURE_NORMAL:
    case eMoveResult::VALID_CAPTURE_ENPASSANT:
        return true;

    case eMoveResult::INVALID_MOVE_BAD_LOCATION:
    case eMoveResult::INVALID_MOVE_NO_PIECE:
    case eMoveResult::INVALID_MOVE_NOT_YOUR_PIECE:
    case eMoveResult::INVALID_MOVE_ZERO_DISTANCE:
    case eMoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE:
    case eMoveResult::INVALID_MOVE_DESTINATION_BLOCKED:
    case eMoveResult::INVALID_MOVE_PATH_BLOCKED:
    case eMoveResult::INVALID_MOVE_ENDS_IN_CHECK:
    case eMoveResult::INVALID_ENPASSANT_STALE:
    case eMoveResult::INVALID_CASTLE_KING_HAS_MOVED:
    case eMoveResult::INVALID_CASTLE_ROOK_HAS_MOVED:
    case eMoveResult::INVALID_CASTLE_PATH_BLOCKED:
    case eMoveResult::INVALID_CASTLE_THROUGH_CHECK:
    case eMoveResult::INVALID_CASTLE_OUT_OF_CHECK:
        return false;

    case eMoveResult::UNKNOWN: ERROR_AND_DIE(Stringf("Unhandled MoveResult enum value #%d", result))
    default: ERROR_AND_DIE(Stringf("Unhandled MoveResult enum value #%d", result))
    }
}
