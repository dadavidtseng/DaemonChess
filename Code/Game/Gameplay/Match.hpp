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
    void RenderGhostPiece() const;

    Board*    m_board = nullptr;
    PieceList m_pieceList;

    void SendChessCommand(const std::string& command);

    bool ValidateGameState(const std::string& state, const std::string& player1, const std::string& player2, int move, const std::string& board);
    void DisconnectWithReason(const std::string& reason);

private:
    void UpdateFromInput(float deltaSeconds);
    void CreateBoard();


    static bool OnEnterMatchState(EventArgs& args);
    static bool OnEnterMatchTurn(EventArgs& args);
    static bool OnExitMatchTurn(EventArgs& args);
    static bool OnMatchInitialized(EventArgs& args);

    void OnChessMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promoteTo, bool isTeleport);
    bool ExecuteMove(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promoteTo, bool isTeleport);

    void        ExecuteEnPassantCapture(IntVec2 const& fromCoords, IntVec2 const& toCoords);
    void        ExecutePawnPromotion(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promoteTo);
    void        ExecuteCastling(IntVec2 const& fromCoords, IntVec2 const& toCoords) const;
    void        ExecuteKingsideCastling(IntVec2 const& fromCoords) const;
    void        ExecuteQueensideCastling(IntVec2 const& fromCoords) const;
    void        RenderPlayerBasis() const;
    static bool OnGameDataReceived(EventArgs& args);

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

    void        RegisterNetworkCommands();
    void        UnregisterNetworkCommands();
    static bool OnChessServerInfo(EventArgs& args);
    static bool OnChessListen(EventArgs& args);
    static bool OnChessConnect(EventArgs& args);
    static bool OnChessDisconnect(EventArgs& args);
    static bool OnChessPlayerInfo(EventArgs& args);
    static bool OnChessBegin(EventArgs& args);
    static bool OnChessValidate(EventArgs& args);
    static bool OnChessMove(EventArgs& args);
    static bool OnChessResign(EventArgs& args);
    static bool OnChessOfferDraw(EventArgs& args);
    static bool OnChessAcceptDraw(EventArgs& args);
    static bool OnChessRejectDraw(EventArgs& args);
    static bool OnRemoteCmd(EventArgs& args);

    // Network state getters
    std::string     GetPlayer1Name() const { return m_player1Name; }
    std::string     GetPlayer2Name() const { return m_player2Name; }
    std::string     GetMyPlayerName() const { return m_myPlayerName; }
    eChessGameState GetGameState() const { return m_gameState; }
    int             GetMoveNumber() const { return m_moveNumber; }
    bool            IsConnected() const { return m_isConnected; }
    std::string     GetBoardStateString() const;
    bool            IsMyTurn() const;

    Camera* m_screenCamera = nullptr;
    Clock*  m_gameClock    = nullptr;

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

    // 網路狀態
    std::string     m_myPlayerName          = "Player";
    std::string     m_player1Name           = "";    // 執白棋的玩家
    std::string     m_player2Name           = "";    // 執黑棋的玩家
    std::string     m_serverIP              = "127.0.0.1";
    int             m_serverPort            = 3100;
    bool            m_isConnected           = false;
    bool            m_isServer              = false;
    bool            m_amIPlayer1            = false;         // 我是否執白棋？
    eChessGameState m_gameState             = eChessGameState::WAITING_FOR_CONNECTION;
    int             m_moveNumber            = 0;              // 從 0 開始計算
    bool            m_drawOffered           = false;
    bool            m_drawOfferFromOpponent = false;
};
