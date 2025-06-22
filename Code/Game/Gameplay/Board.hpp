//----------------------------------------------------------------------------------------------------
// Board.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Gameplay/Actor.hpp"

//----------------------------------------------------------------------------------------------------
class Match;
class Piece;
class Texture;
struct BoardDefinition;

//----------------------------------------------------------------------------------------------------
class Board final : public Actor
{
public:
    explicit Board(Match* owner, Texture const* texture = nullptr);
    ~Board() override;

    void Update(float deltaSeconds) override;
    void Render() const override;

    /// Query
    Vec3        GetWorldPositionByCoords(IntVec2 const& coords);
    Piece*      GetPieceByCoords(IntVec2 const& coords) const;
    sSquareInfo GetSquareInfoByCoords(IntVec2 const& coords);
    IntVec2     StringToChessCoord(String const& chessPos);
    String      ChessCoordToString(IntVec2 const& coords);
    String      GetBoardContents(int rowNum) const;
    bool        IsCoordValid(IntVec2 const& coords) const;

    /// Render
    void CreateLocalVertsForAABB3s();
    void CreateLocalVertsForBoardFrame();

    /// Mutators (non-const methods)
    void UpdateSquareInfoList(IntVec2 const& toCoords);
    void UpdateSquareInfoList(IntVec2 const& fromCoords, IntVec2 const& toCoords);
    void UpdateSquareInfoList(IntVec2 const& fromCoords, IntVec2 const& toCoords, String const& promoteTo);

    IntVec2 FindKingCoordsByPlayerId(int playerId) const;

    std::vector<sSquareInfo> m_squareInfoList;

private:
    BoardDefinition*  m_definition = nullptr;
    VertexList_PCUTBN m_vertexes;
    IndexList         m_indexes;
    Texture const*    m_texture = nullptr;
    Shader const*     m_shader  = nullptr;
};
