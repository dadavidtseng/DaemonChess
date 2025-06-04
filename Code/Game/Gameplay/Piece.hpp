//----------------------------------------------------------------------------------------------------
// Prop.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Game/Gameplay/Actor.hpp"

struct sSquareInfo;
struct PieceDefinition;
//----------------------------------------------------------------------------------------------------
class Texture;
struct Vertex_PCU;

//----------------------------------------------------------------------------------------------------
/// @brief
/// Owned by Match, inherits Actor
class Piece final : public Actor
{
public:
    explicit Piece(Match* owner, sSquareInfo const& squareInfo, Texture const* texture = nullptr);

    void Update(float deltaSeconds) override;
    void UpdatePositionByCoords(IntVec2 const& newCoords);

    void Render() const override;

    // void InitializeLocalVertsForGrid();
    // void InitializeLocalVertsForCylinder();
    // void InitializeLocalVertsForWorldCoordinateArrows();
    // void InitializeLocalVertsForText2D();

private:
    VertexList_PCUTBN m_vertexes;
    IndexList         m_indexes;

    Texture const*   m_texture    = nullptr;
    PieceDefinition* m_definition = nullptr;
    Shader*          m_shader     = nullptr;
};
