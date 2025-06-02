//----------------------------------------------------------------------------------------------------
// Board.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Game/Gameplay/Actor.hpp"

class Match;
//----------------------------------------------------------------------------------------------------
class Texture;
struct Vertex_PCU;

//----------------------------------------------------------------------------------------------------
class Board final : public Actor
{
public:
    explicit Board(Match* owner, Texture const* texture = nullptr);

    void Update(float deltaSeconds) override;
    void Render() const override;
    void InitializeLocalVertsForCube();
    void InitializeLocalVertsForSphere();
    void InitializeLocalVertsForGrid();
    void InitializeLocalVertsForCylinder();
    void InitializeLocalVertsForWorldCoordinateArrows();
    void InitializeLocalVertsForText2D();

private:
    std::vector<Vertex_PCU> m_vertexes;
    Texture const* m_texture = nullptr;
};
