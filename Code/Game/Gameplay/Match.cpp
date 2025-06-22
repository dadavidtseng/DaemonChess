//----------------------------------------------------------------------------------------------------
// Match.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Match.hpp"

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Definition/PieceDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/MatchCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Piece.hpp"

//----------------------------------------------------------------------------------------------------
Match::Match()
{
    g_theEventSystem->SubscribeEventCallbackFunction("ChessMove", OnChessMove);
    g_theEventSystem->SubscribeEventCallbackFunction("OnGameStateChanged", OnEnterMatchState);
    g_theEventSystem->SubscribeEventCallbackFunction("OnEnterMatchTurn", OnEnterMatchTurn);
    g_theEventSystem->SubscribeEventCallbackFunction("OnExitMatchTurn", OnExitMatchTurn);
    g_theEventSystem->SubscribeEventCallbackFunction("OnMatchInitialized", OnMatchInitialized);

    m_screenCamera = new Camera();

    Vec2 const bottomLeft     = Vec2::ZERO;
    Vec2 const screenTopRight = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);

    m_screenCamera->SetOrthoGraphicView(bottomLeft, screenTopRight);
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    m_gameClock = new Clock(Clock::GetSystemClock());

    CreateBoard();
    for (PieceDefinition* pieceDef : PieceDefinition::s_pieceDefinitions)
    {
        pieceDef->CreateMeshByID(0);
        pieceDef->CreateMeshByID(1);
    }

    for (BoardDefinition const* boardDefs : BoardDefinition::s_boardDefinitions)
    {
        for (sSquareInfo const& squareInfo : boardDefs->m_squareInfos)
        {
            m_board->m_squareInfoList.push_back(squareInfo);

            if (squareInfo.m_name == "DEFAULT") continue;

            Piece* piece         = new Piece(this, squareInfo);
            piece->m_orientation = boardDefs->m_pieceOrientation;
            piece->m_color       = boardDefs->m_pieceColor;
            m_pieceList.push_back(piece);
        }
    }

#if defined DEBUG_MODE
    DebugAddWorldBasis(Mat44(), -1.f);

    Mat44 transform;

    transform.SetIJKT3D(-Vec3::Y_BASIS, Vec3::X_BASIS, Vec3::Z_BASIS, Vec3(0.25f, 0.f, 0.25f));
    DebugAddWorldText("X-Forward", transform, 0.25f, Vec2::ONE, -1.f, Rgba8::RED);

    transform.SetIJKT3D(-Vec3::X_BASIS, -Vec3::Y_BASIS, Vec3::Z_BASIS, Vec3(0.f, 0.25f, 0.5f));
    DebugAddWorldText("Y-Left", transform, 0.25f, Vec2::ZERO, -1.f, Rgba8::GREEN);

    transform.SetIJKT3D(-Vec3::X_BASIS, Vec3::Z_BASIS, Vec3::Y_BASIS, Vec3(0.f, -0.25f, 0.25f));
    DebugAddWorldText("Z-Up", transform, 0.25f, Vec2(1.f, 0.f), -1.f, Rgba8::BLUE);
#endif
}

//----------------------------------------------------------------------------------------------------
Match::~Match()
{
    SafeDeletePointer(m_screenCamera);
    SafeDeletePointer(m_board);

    for (int i = 0; i < static_cast<int>(m_pieceList.size()); ++i)
    {
        SafeDeletePointer(m_pieceList[i]);
    }
    m_pieceList.clear();
}

void Match::Update()
{
    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    DebugAddScreenText(Stringf("Time: %.2f\nFPS: %.2f\nScale: %.1f", m_gameClock->GetTotalSeconds(), 1.f / m_gameClock->GetDeltaSeconds(), m_gameClock->GetTimeScale()), m_screenCamera->GetOrthographicTopRight() - Vec2(250.f, 60.f), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);

    UpdateFromInput(deltaSeconds);

    m_board->Update(deltaSeconds);
}

void Match::UpdateFromInput(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}

//----------------------------------------------------------------------------------------------------
void Match::Render() const
{
    g_theRenderer->SetLightConstants(m_sunDirection, m_sunIntensity, m_ambientIntensity);

    m_board->Render();

    for (Piece const* piece : m_pieceList)
    {
        if (piece == nullptr) continue;
        piece->Render();
    }
}

//----------------------------------------------------------------------------------------------------
void Match::CreateBoard()
{
    Texture const* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/WhiteMarble_COLOR.jpg");

    m_board = new Board(this, texture);
}

//----------------------------------------------------------------------------------------------------
bool Match::IsChessMoveValid(IntVec2 const& fromCoords,
                             IntVec2 const& toCoords) const
{
    MoveResult const result = ValidateChessMove(fromCoords, toCoords);

    if (!IsMoveValid(result))
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, GetMoveResultString(result));
        return false;
    }

    return true;
}

void Match::HandleCapture(IntVec2 const& fromCoords, IntVec2 const& toCoords) const
{
    Piece const* fromPiece = m_board->GetPieceByCoords(fromCoords);
    Piece const* toPiece   = m_board->GetPieceByCoords(toCoords);

    if (toPiece == nullptr) return;

    // If captured piece is a king, end the match
    if (toPiece->m_definition->m_type == ePieceType::KING)
    {
        m_board->CapturePiece(fromPiece->m_coords, toPiece->m_coords);

        g_theDevConsole->AddLine(DevConsole::WARNING, "##################################################");
        g_theDevConsole->AddLine(DevConsole::WARNING, Stringf("[SYSTEM] Player #%d has won the match!", g_theGame->GetCurrentPlayerControllerId()));
        g_theDevConsole->AddLine(DevConsole::WARNING, "##################################################");
        g_theGame->ChangeGameState(eGameState::FINISHED);
    }
    else
    {
        m_board->CapturePiece(fromPiece->m_coords, toPiece->m_coords);
    }
}

void Match::OnChessMove(IntVec2 const& fromCoords,
                        IntVec2 const& toCoords,
                        String const&  promotionType,
                        bool const     isTeleport)
{
    // if (isTeleport)
    // {
    //     g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("Move Player #%d's %s from %s to %s", g_theGame->GetCurrentPlayerControllerId(), m_board->GetPieceByCoords(fromCoords)->m_definition->m_name.c_str(), m_board->ChessCoordToString(fromCoords).c_str(),
    //                                                          m_board->ChessCoordToString(toCoords).c_str()));
    //     // ExecuteMove(fromCoords, toCoords, promotionType);
    //
    //     g_theEventSystem->FireEvent("OnExitMatchTurn");
    //     return;
    // }
    if (!IsChessMoveValid(fromCoords, toCoords)) return;

    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("Move Player #%d's %s from %s to %s", g_theGame->GetCurrentPlayerControllerId(), m_board->GetPieceByCoords(fromCoords)->m_definition->m_name.c_str(), m_board->ChessCoordToString(fromCoords).c_str(),
                                                             m_board->ChessCoordToString(toCoords).c_str()));
    ExecuteMove(fromCoords, toCoords, promotionType);

    g_theEventSystem->FireEvent("OnExitMatchTurn");
    // TODO: Add teleport event to cheat
}

void Match::UpdatePieceList(IntVec2 const& fromCoords,
                            IntVec2 const& toCoords)
{
    UNUSED(fromCoords)
    for (auto it = m_pieceList.begin(); it != m_pieceList.end();)
    {
        Piece* piece = *it;

        if (piece->m_coords == toCoords)
        {
            delete piece;

            it = m_pieceList.erase(it);
            break;
        }

        ++it;
    }
}

void Match::RemovePieceList(IntVec2 const& toCoords)
{
    for (auto it = m_pieceList.begin(); it != m_pieceList.end();)
    {
        Piece* piece = *it;

        if (piece->m_coords == toCoords)
        {
            delete piece;

            it = m_pieceList.erase(it);
            break;
        }

        ++it;
    }
}

bool Match::OnChessMove(EventArgs& args)
{
    String const from       = args.GetValue("from", "DEFAULT");
    String const to         = args.GetValue("to", "DEFAULT");
    String const promotion  = args.GetValue("promoteTo", "DEFAULT");
    bool const   isTeleport = args.GetValue("teleport", false);

    IntVec2 const fromCoords = g_theGame->m_match->m_board->StringToChessCoord(from);
    IntVec2 const toCoords   = g_theGame->m_match->m_board->StringToChessCoord(to);

    g_theGame->m_match->OnChessMove(fromCoords, toCoords, promotion, isTeleport);
    return true;
}

bool Match::OnEnterMatchState(EventArgs& args)
{
    OnEnterMatchTurn(args);
    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnEnterMatchTurn(EventArgs& args)
{
    UNUSED(args)

    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("=================================================="));
    g_theDevConsole->AddLine(DevConsole::INFO_MINOR, Stringf("Player #%d -- it's your turn!", g_theGame->GetCurrentPlayerControllerId()));

    int const currentTurnPlayerIndex = g_theGame->GetCurrentPlayerControllerId();

    if (currentTurnPlayerIndex == 0 || currentTurnPlayerIndex == -1) g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Game state is: First Player's Turn"));
    else if (currentTurnPlayerIndex == 1) g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Game state is: Second Player's Turn"));

    g_theDevConsole->AddLine(DevConsole::INPUT_TEXT, Stringf("  ABCDEFGH"));
    g_theDevConsole->AddLine(DevConsole::INPUT_TEXT, Stringf(" +--------+"));

    for (int row = 8; row >= 1; --row)
    {
        g_theDevConsole->AddLine(DevConsole::INPUT_TEXT, Stringf("%d|%s|%d", row, g_theGame->m_match->m_board->GetBoardContents(row).c_str(), row));
    }

    g_theDevConsole->AddLine(DevConsole::INPUT_TEXT, Stringf(" +--------+"));
    g_theDevConsole->AddLine(DevConsole::INPUT_TEXT, Stringf("  ABCDEFGH"));

    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnExitMatchTurn(EventArgs& args)
{
    UNUSED(args)

    g_theGame->TogglePlayerControllerId();
    g_theEventSystem->FireEvent("OnEnterMatchTurn");
    return true;
}

//----------------------------------------------------------------------------------------------------
bool Match::OnMatchInitialized(EventArgs& args)
{
    UNUSED(args)
    g_theEventSystem->FireEvent("OnEnterMatchTurn");
    return true;
}

MoveResult Match::ValidateChessMove(IntVec2 const&     fromCoords,
                                    IntVec2 const&     toCoords,
                                    std::string const& promotionType) const
{
    // 1. Check if coordinates are valid
    if (!m_board->IsCoordValid(fromCoords) || !m_board->IsCoordValid(toCoords))
    {
        return MoveResult::INVALID_MOVE_BAD_LOCATION;
    }

    // 2. Check if source square has a piece
    Piece const* fromPiece = m_board->GetPieceByCoords(fromCoords);

    if (fromPiece == nullptr)
    {
        return MoveResult::INVALID_MOVE_NO_PIECE;
    }

    // 3. Check if piece belongs to current player
    if (m_board->GetSquareInfoByCoords(fromCoords).m_playerControllerId != g_theGame->GetCurrentPlayerControllerId())
    {
        return MoveResult::INVALID_MOVE_NOT_YOUR_PIECE;
    }

    // 4. Check if trying to move to same square
    if (fromCoords == toCoords)
    {
        return MoveResult::INVALID_MOVE_ZERO_DISTANCE;
    }

    // 5. Check destination square
    Piece const* toPiece = m_board->GetPieceByCoords(toCoords);
    int const    toOwner = m_board->GetSquareInfoByCoords(toCoords).m_playerControllerId;

    if (toPiece != nullptr && toOwner == g_theGame->GetCurrentPlayerControllerId())
    {
        return MoveResult::INVALID_MOVE_DESTINATION_BLOCKED;
    }

    // 6. Check piece-specific movement rules
    MoveResult pieceValidation = ValidatePieceMovement(fromCoords, toCoords, promotionType);
    if (pieceValidation != MoveResult::VALID_MOVE_NORMAL)
    {
        return pieceValidation;
    }

    // 7. Check if sliding pieces are blocked
    if (!IsPathClear(fromCoords, toCoords, fromPiece->m_definition->m_type))
    {
        return MoveResult::INVALID_MOVE_PATH_BLOCKED;
    }

    // 8. Kings apart rule - king cannot move adjacent to enemy king
    if (fromPiece->m_definition->m_type == ePieceType::KING)
    {
        if (!IsKingDistanceValid(toCoords))
        {
            return MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
        }
    }

    // Determine the type of valid move
    return DetermineValidMoveType(fromCoords, toCoords, fromPiece, promotionType);
}

MoveResult Match::ValidatePieceMovement(IntVec2 const&     fromCoords,
                                        IntVec2 const&     toCoords,
                                        std::string const& promotionType) const
{
    Piece const* fromPiece = m_board->GetPieceByCoords(fromCoords);
    ePieceType   pieceType = fromPiece->m_definition->m_type;

    int deltaX    = toCoords.x - fromCoords.x;
    int deltaY    = toCoords.y - fromCoords.y;
    int absDeltaX = abs(deltaX);
    int absDeltaY = abs(deltaY);

    switch (pieceType)
    {
    case ePieceType::PAWN: return ValidatePawnMove(fromCoords, toCoords, promotionType);
    case ePieceType::ROOK: return ValidateRookMove(deltaX, deltaY);
    case ePieceType::BISHOP: return ValidateBishopMove(absDeltaX, absDeltaY);
    case ePieceType::KNIGHT: return ValidateKnightMove(absDeltaX, absDeltaY);
    case ePieceType::QUEEN: return ValidateQueenMove(deltaX, deltaY, absDeltaX, absDeltaY);
    case ePieceType::KING: return ValidateKingMove(absDeltaX, absDeltaY, fromCoords, toCoords);
    case ePieceType::NONE:
    default: return MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
    }
}

MoveResult Match::ValidatePawnMove(IntVec2 const&     fromCoords,
                                   IntVec2 const&     toCoords,
                                   std::string const& promotionType) const
{
    Piece const* fromPiece = m_board->GetPieceByCoords(fromCoords);
    Piece const* toPiece   = m_board->GetPieceByCoords(toCoords);

    int currentPlayer = g_theGame->GetCurrentPlayerControllerId();
    int direction     = (currentPlayer == 0) ? 1 : -1; // Player 0 moves up, Player 1 moves down

    int deltaX = toCoords.x - fromCoords.x;
    int deltaY = toCoords.y - fromCoords.y;

    // Check for pawn promotion
    int promotionRank = (currentPlayer == 0) ? 8 : 1;
    if (toCoords.y == promotionRank)
    {
        if (promotionType.empty())
        {
            return MoveResult::VALID_MOVE_PROMOTION; // Need promotion parameter
        }

        if (!IsValidPromotionType(promotionType))
        {
            return MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
        }
    }

    // Forward movement (1 or 2 squares)
    if (deltaX == 0 && toPiece == nullptr)
    {
        if (deltaY == direction) // 1 square forward
        {
            return MoveResult::VALID_MOVE_NORMAL;
        }
        else if (deltaY == 2 * direction) // 2 squares forward
        {
            // Check if pawn is in starting position or has never moved
            int startingRank = (currentPlayer == 0) ? 1 : 6;
            if (fromCoords.y == startingRank || !fromPiece->m_hasMoved)
            {
                return MoveResult::VALID_MOVE_NORMAL;
            }
            else
            {
                return MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
            }
        }
    }
    // Diagonal capture
    else if (abs(deltaX) == 1 && deltaY == direction)
    {
        if (toPiece != nullptr)
        {
            return MoveResult::VALID_CAPTURE_NORMAL; // Normal capture
        }
        else
        {
            // Check for en passant
            if (IsValidEnPassant(fromCoords, toCoords))
            {
                return MoveResult::VALID_CAPTURE_ENPASSANT;
            }
            else
            {
                return MoveResult::INVALID_ENPASSANT_STALE;
            }
        }
    }

    return MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

MoveResult Match::ValidateRookMove(int deltaX, int deltaY) const
{
    if ((deltaX == 0 && deltaY != 0) || (deltaX != 0 && deltaY == 0))
    {
        return MoveResult::VALID_MOVE_NORMAL;
    }

    return MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

MoveResult Match::ValidateBishopMove(int absDeltaX, int absDeltaY) const
{
    if (absDeltaX == absDeltaY && absDeltaX > 0)
    {
        return MoveResult::VALID_MOVE_NORMAL;
    }

    return MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

MoveResult Match::ValidateKnightMove(int absDeltaX, int absDeltaY) const
{
    if ((absDeltaX == 2 && absDeltaY == 1) || (absDeltaX == 1 && absDeltaY == 2))
    {
        return MoveResult::VALID_MOVE_NORMAL;
    }

    return MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

MoveResult Match::ValidateQueenMove(int deltaX, int deltaY, int absDeltaX, int absDeltaY) const
{
    // Queen moves like rook or bishop
    bool isRookMove   = (deltaX == 0 && deltaY != 0) || (deltaX != 0 && deltaY == 0);
    bool isBishopMove = (absDeltaX == absDeltaY && absDeltaX > 0);

    if (isRookMove || isBishopMove)
    {
        return MoveResult::VALID_MOVE_NORMAL;
    }

    return MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

MoveResult Match::ValidateKingMove(int            absDeltaX, int absDeltaY,
                                   IntVec2 const& fromCoords,
                                   IntVec2 const& toCoords) const
{
    // Check for castling
    if (absDeltaY == 0 && absDeltaX == 3)
    {
        return ValidateCastling(fromCoords, toCoords);
    }

    if (absDeltaY == 0 && absDeltaX == 4)
    {
        return ValidateCastling(fromCoords, toCoords);
    }

    // Normal king move (1 square in any direction)
    if (absDeltaX <= 1 && absDeltaY <= 1)
    {
        return MoveResult::VALID_MOVE_NORMAL;
    }

    return MoveResult::INVALID_MOVE_WRONG_MOVE_SHAPE;
}

bool Match::IsKingDistanceValid(IntVec2 const& toCoords) const
{
    // Find enemy king position
    int     enemyPlayer  = 1 - g_theGame->GetCurrentPlayerControllerId();
    IntVec2 enemyKingPos = m_board->FindKingPosition(enemyPlayer);

    // Check if destination is adjacent to enemy king
    int deltaX = abs(toCoords.x - enemyKingPos.x);
    int deltaY = abs(toCoords.y - enemyKingPos.y);

    if (deltaX <= 1 && deltaY <= 1 && !(deltaX == 0 && deltaY == 0))
    {
        return false; // Kings cannot be adjacent
    }

    return true;
}

bool Match::IsPathClear(IntVec2 const& fromCoords, IntVec2 const& toCoords, ePieceType const& pieceType) const
{
    // Knights don't need clear path
    if (pieceType == ePieceType::KNIGHT)
    {
        return true;
    }

    // Kings moving 1 square don't need path check
    if (pieceType == ePieceType::KING)
    {
        int absDeltaX = abs(toCoords.x - fromCoords.x);
        int absDeltaY = abs(toCoords.y - fromCoords.y);
        if (absDeltaX <= 1 && absDeltaY <= 1)
        {
            return true;
        }
    }

    int deltaX = toCoords.x - fromCoords.x;
    int deltaY = toCoords.y - fromCoords.y;

    int stepX = (deltaX == 0) ? 0 : (deltaX > 0) ? 1 : -1;
    int stepY = (deltaY == 0) ? 0 : (deltaY > 0) ? 1 : -1;

    IntVec2 currentPos = fromCoords;
    currentPos.x += stepX;
    currentPos.y += stepY;

    while (currentPos != toCoords)
    {
        if (m_board->GetPieceByCoords(currentPos) != nullptr)
        {
            return false;
        }

        currentPos.x += stepX;
        currentPos.y += stepY;
    }

    return true;
}

bool Match::IsValidEnPassant(IntVec2 const& fromCoords, IntVec2 const& toCoords) const
{
    PieceMove const lastMove = GetLastPieceMove();

    // Check if last move was a pawn moving 2 squares
    if (lastMove.piece == nullptr || lastMove.piece->m_definition->m_type != ePieceType::PAWN)
    {
        return false;
    }

    int lastMoveDelta = abs(lastMove.toCoords.y - lastMove.fromCoords.y);
    if (lastMoveDelta != 2)
    {
        return false;
    }

    // Check if the pawn to be captured is adjacent and the target square is the "passed through" square
    IntVec2 capturedPawnPos = IntVec2(toCoords.x, fromCoords.y);
    if (lastMove.toCoords == capturedPawnPos)
    {
        IntVec2 passedThroughSquare = IntVec2(lastMove.fromCoords.x, (lastMove.fromCoords.y + lastMove.toCoords.y) / 2);
        if (toCoords == passedThroughSquare)
        {
            return true;
        }
    }

    return false;
}

MoveResult Match::ValidateCastling(IntVec2 const& fromCoords, IntVec2 const& toCoords) const
{
    Piece const* king = m_board->GetPieceByCoords(fromCoords);

    // King must not have moved
    if (king->m_hasMoved)
    {
        return MoveResult::INVALID_CASTLE_KING_HAS_MOVED;
    }

    // Determine castling side
    bool    isKingSide = toCoords.x > fromCoords.x;
    IntVec2 rookPos    = IntVec2(isKingSide ? 8 : 1, fromCoords.y);

    Piece const* rook = m_board->GetPieceByCoords(rookPos);
    if (rook == nullptr || rook->m_definition->m_type != ePieceType::ROOK)
    {
        return MoveResult::INVALID_CASTLE_ROOK_HAS_MOVED;
    }

    // Rook must not have moved
    if (rook->m_hasMoved)
    {
        return MoveResult::INVALID_CASTLE_ROOK_HAS_MOVED;
    }

    // Check if path is clear between king and rook
    int startX = std::min(fromCoords.x, rookPos.x) + 1;
    int endX   = std::max(fromCoords.x, rookPos.x);

    for (int x = startX; x < endX; ++x)
    {
        if (m_board->GetPieceByCoords(IntVec2(x, fromCoords.y)) != nullptr)
        {
            return MoveResult::INVALID_CASTLE_PATH_BLOCKED;
        }
    }

    // TODO: Add check-related castle validation in future assignments
    // - King cannot be in check
    // - King cannot pass through check
    // - King cannot end in check

    return isKingSide ? MoveResult::VALID_CASTLE_KINGSIDE : MoveResult::VALID_CASTLE_QUEENSIDE;
}

bool Match::IsValidPromotionType(std::string const& promotionType) const
{
    return (promotionType == "queen" || promotionType == "rook" ||
        promotionType == "bishop" || promotionType == "knight");
}

MoveResult Match::DetermineValidMoveType(IntVec2 const&     fromCoords,
                                         IntVec2 const&     toCoords,
                                         Piece const*       fromPiece,
                                         std::string const& promotionType) const
{
    Piece const* toPiece = m_board->GetPieceByCoords(toCoords);

    // Check for pawn promotion
    if (fromPiece->m_definition->m_type == ePieceType::PAWN)
    {
        int currentPlayer = g_theGame->GetCurrentPlayerControllerId();
        int promotionRank = (currentPlayer == 0) ? 8 : 1;

        if (toCoords.y == promotionRank)
        {
            if (toPiece != nullptr)
            {
                return MoveResult::VALID_MOVE_PROMOTION; // Promotion with capture
            }
            else
            {
                return MoveResult::VALID_MOVE_PROMOTION; // Promotion without capture
            }
        }

        // Check for en passant (already validated in pawn move)
        if (abs(toCoords.x - fromCoords.x) == 1 && toPiece == nullptr)
        {
            return MoveResult::VALID_CAPTURE_ENPASSANT;
        }
    }

    // Check for castling
    if (fromPiece->m_definition->m_type == ePieceType::KING)
    {
        int absDeltaX = abs(toCoords.x - fromCoords.x);
        if (absDeltaX == 2)
        {
            bool isKingSide = toCoords.x > fromCoords.x;
            return isKingSide ? MoveResult::VALID_CASTLE_KINGSIDE : MoveResult::VALID_CASTLE_QUEENSIDE;
        }
    }

    // Check for capture
    if (toPiece != nullptr)
    {
        return MoveResult::VALID_CAPTURE_NORMAL;
    }

    // Normal move
    return MoveResult::VALID_MOVE_NORMAL;
}

PieceMove Match::GetLastPieceMove() const
{
    return m_pieceMoveList.back();
}

void Match::ExecuteMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, std::string const& promotionType)
{
    MoveResult result = ValidateChessMove(fromCoords, toCoords, promotionType);

    if (!IsMoveValid(result))
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, GetMoveResultString(result));
        return;
    }

    Piece* fromPiece = m_board->GetPieceByCoords(fromCoords);

    // Handle special moves based on result type
    switch (result)
    {
    case MoveResult::VALID_CAPTURE_ENPASSANT:
        ExecuteEnPassantCapture(fromCoords, toCoords);
        break;

    case MoveResult::VALID_MOVE_PROMOTION:
        ExecutePawnPromotion(fromCoords, toCoords, promotionType);
        break;

    case MoveResult::VALID_CASTLE_KINGSIDE:
    case MoveResult::VALID_CASTLE_QUEENSIDE:
        ExecuteCastling(fromCoords, toCoords);
        break;

    case MoveResult::VALID_CAPTURE_NORMAL:
        // HandleCapture(fromCoords, toCoords);
        // m_board->MovePiece(fromCoords, toCoords);
        fromPiece->UpdatePositionByCoords(toCoords);
        fromPiece->m_hasMoved = true;
        m_board->UpdateSquareInfoList(fromCoords, toCoords);

        break;
    case MoveResult::VALID_MOVE_NORMAL:
    default:
        fromPiece->UpdatePositionByCoords(toCoords);
        fromPiece->m_hasMoved = true;
        m_board->UpdateSquareInfoList(fromCoords, toCoords);

        break;
    }

    // Record move for en passant detection
    m_pieceMoveList.push_back({fromPiece, fromCoords, toCoords});

    g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, GetMoveResultString(result));
}

void Match::ExecuteEnPassantCapture(IntVec2 const& fromCoords, IntVec2 const& toCoords)
{
    // Remove the captured pawn
    IntVec2 capturedPawnPos = IntVec2(toCoords.x, fromCoords.y);
    Piece*  fromPiece       = m_board->GetPieceByCoords(fromCoords);
    fromPiece->UpdatePositionByCoords(toCoords);
    m_board->UpdateSquareInfoList(fromCoords, toCoords);
    m_board->RemovePiece(capturedPawnPos);
    RemovePieceList(capturedPawnPos);
    // Move the capturing pawn
    // m_board->MovePiece(fromCoords, toCoords);
}

void Match::ExecutePawnPromotion(IntVec2 const& fromCoords, IntVec2 const& toCoords, std::string const& promotionType) const
{
    // Handle capture if there's a piece at destination
    HandleCapture(fromCoords, toCoords);

    Piece* fromPiece        = m_board->GetPieceByCoords(fromCoords);
    fromPiece->m_definition = PieceDefinition::GetDefByName(promotionType);
    fromPiece->UpdatePositionByCoords(toCoords);
    // fromPiece->m_hasMoved = true;
    m_board->UpdateSquareInfoList(fromCoords, toCoords, promotionType);
    // m_board->UpdateBoardSquareInfoList(fromCoords, toCoords);
    // Promote the pawn
    m_board->PromotePawn(fromCoords, toCoords, promotionType);
}

void Match::ExecuteCastling(IntVec2 const& fromCoords, IntVec2 const& toCoords) const
{
    bool    isKingSide  = toCoords.x > fromCoords.x;
    IntVec2 kingToPos   = IntVec2(isKingSide ? 7 : 3, fromCoords.y);
    IntVec2 rookFromPos = IntVec2(isKingSide ? 8 : 1, fromCoords.y);
    IntVec2 rookToPos   = IntVec2(isKingSide ? 6 : 4, fromCoords.y);

    Piece* king = m_board->GetPieceByCoords(fromCoords);
    king->UpdatePositionByCoords(kingToPos);  // 不是 toCoords！
    m_board->UpdateSquareInfoList(fromCoords, kingToPos);

    // Move rook
    Piece* rook = m_board->GetPieceByCoords(rookFromPos);
    rook->UpdatePositionByCoords(rookToPos);
    m_board->UpdateSquareInfoList(rookFromPos, rookToPos);
}
