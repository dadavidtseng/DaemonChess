//----------------------------------------------------------------------------------------------------
// MatchCommon.cpp
//----------------------------------------------------------------------------------------------------

#include "Game/Framework/MatchCommon.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
char const* GetMoveResultString(MoveResult const& result)
{
    switch (result)
    {
    case MoveResult::UNKNOWN: return "Unknown ChessMoveResult!";
    case MoveResult::VALID_MOVE_NORMAL: return "Valid move";
    case MoveResult::VALID_MOVE_PROMOTION: return "Valid move, resulting in pawn promotion";
    case MoveResult::VALID_CASTLE_KINGSIDE: return "Valid move, castling kingside";
    case MoveResult::VALID_CASTLE_QUEENSIDE: return "Valid move, castling queenside";
    case MoveResult::VALID_CAPTURE_NORMAL: return "Valid move, capturing enemy piece";
    case MoveResult::VALID_CAPTURE_ENPASSANT: return "Valid move, capturing enemy pawn en passant";
    case MoveResult::INVALID_MOVE_BAD_LOCATION: return "Invalid move; invalid board location given";
    case MoveResult::INVALID_MOVE_NO_PIECE: return "Invalid move; no piece at location given";
    case MoveResult::INVALID_MOVE_NOT_YOUR_PIECE: return "Invalid move; can't move opponent's piece";
    case MoveResult::INVALID_MOVE_ZERO_DISTANCE: return "Invalid move; didn't go anywhere";
    case MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE: return "Invalid move; piece cannot move in that way";
    case MoveResult::INVALID_MOVE_DESTINATION_BLOCKED: return "Invalid move; destination is blocked by your piece";
    case MoveResult::INVALID_MOVE_PATH_BLOCKED: return "Invalid move; path is blocked by your piece";
    case MoveResult::INVALID_MOVE_ENDS_IN_CHECK: return "Invalid move; can't leave yourself in check";
    case MoveResult::INVALID_ENPASSANT_STALE: return "Invalid move; en passant must immediately follow a pawn double-move";
    case MoveResult::INVALID_CASTLE_KING_HAS_MOVED: return "Invalid castle; king has moved previously";
    case MoveResult::INVALID_CASTLE_ROOK_HAS_MOVED: return "Invalid castle; that rook has moved previously";
    case MoveResult::INVALID_CASTLE_PATH_BLOCKED: return "Invalid castle; pieces in-between king and rook";
    case MoveResult::INVALID_CASTLE_THROUGH_CHECK: return "Invalid castle; king can't move through check";
    case MoveResult::INVALID_CASTLE_OUT_OF_CHECK: return "Invalid castle; king can't castle out of check";
    default: ERROR_AND_DIE(Stringf("Unhandled MoveResult enum value #%d", result))
    }
}

bool IsMoveValid(MoveResult const& result)
{
    switch (result)
    {
    case MoveResult::VALID_MOVE_NORMAL:
    case MoveResult::VALID_MOVE_PROMOTION:
    case MoveResult::VALID_CASTLE_KINGSIDE:
    case MoveResult::VALID_CASTLE_QUEENSIDE:
    case MoveResult::VALID_CAPTURE_NORMAL:
    case MoveResult::VALID_CAPTURE_ENPASSANT:
        return true;

    case MoveResult::INVALID_MOVE_BAD_LOCATION:
    case MoveResult::INVALID_MOVE_NO_PIECE:
    case MoveResult::INVALID_MOVE_NOT_YOUR_PIECE:
    case MoveResult::INVALID_MOVE_ZERO_DISTANCE:
    case MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE:
    case MoveResult::INVALID_MOVE_DESTINATION_BLOCKED:
    case MoveResult::INVALID_MOVE_PATH_BLOCKED:
    case MoveResult::INVALID_MOVE_ENDS_IN_CHECK:
    case MoveResult::INVALID_ENPASSANT_STALE:
    case MoveResult::INVALID_CASTLE_KING_HAS_MOVED:
    case MoveResult::INVALID_CASTLE_ROOK_HAS_MOVED:
    case MoveResult::INVALID_CASTLE_PATH_BLOCKED:
    case MoveResult::INVALID_CASTLE_THROUGH_CHECK:
    case MoveResult::INVALID_CASTLE_OUT_OF_CHECK:
        return false;

    case MoveResult::UNKNOWN: ERROR_AND_DIE(Stringf("Unhandled MoveResult enum value #%d", result))
    default: ERROR_AND_DIE(Stringf("Unhandled MoveResult enum value #%d", result))
    }
}
