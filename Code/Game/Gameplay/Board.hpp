//----------------------------------------------------------------------------------------------------
// Board.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Game/Gameplay/Actor.hpp"

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

    void InitializeLocalVertsForAABB3s();
    void InitializeLocalVertsForCube();
    void InitializeLocalVertsForSphere();
    void InitializeLocalVertsForGrid();
    void InitializeLocalVertsForCylinder();
    void InitializeLocalVertsForWorldCoordinateArrows();
    void InitializeLocalVertsForText2D();

private:
    VertexList_PCUTBN m_vertexes;
    IndexList         m_indexes;
    PieceList         m_pieces;
    Texture const*    m_texture = nullptr;
    Shader const*     m_shader  = nullptr;
};
