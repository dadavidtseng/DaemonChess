//----------------------------------------------------------------------------------------------------
// Board.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Game/Gameplay/Actor.hpp"

struct BoardDefinition;
//----------------------------------------------------------------------------------------------------
class Match;
class Piece;
class Texture;

//----------------------------------------------------------------------------------------------------
typedef std::vector<Piece*> PieceList;

//----------------------------------------------------------------------------------------------------
class Board final : public Actor
{
public:
    explicit Board(Match* owner, Texture const* texture = nullptr);

    void Update(float deltaSeconds) override;
    void Render() const override;

    /// Query
    IntVec2 GetCoordsByWorldPosition(Vec3 const& worldPosition);
    Vec3    GetWorldPositionByCoords(IntVec2 const& coords);
    Piece* GetPieceByCoords(IntVec2 const& coords);
    IntVec2 StringToChessCoord(String const & chessPos);

    void InitializeLocalVertsForAABB3s();
    void InitializeLocalVertsForGrid();
    PieceList         m_pieceList;
private:
    BoardDefinition*  m_definition = nullptr;
    VertexList_PCUTBN m_vertexes;
    IndexList         m_indexes;

    Texture const*    m_texture = nullptr;
    Shader const*     m_shader  = nullptr;

};
