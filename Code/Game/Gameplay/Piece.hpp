//----------------------------------------------------------------------------------------------------
// Piece.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Game/Gameplay/Actor.hpp"


//----------------------------------------------------------------------------------------------------
struct PieceDefinition;
class Texture;
struct sSquareInfo;
struct Vertex_PCU;

//----------------------------------------------------------------------------------------------------
/// @brief
/// Owned by Match, inherits Actor
class Piece final : public Actor
{
public:
    explicit Piece(Match* owner, sSquareInfo const& squareInfo, Texture const* texture = nullptr);

    void Update(float deltaSeconds) override;
    void Render() const override;

    void UpdatePositionByCoords(IntVec2 const& newCoords);

private:
    VertexList_PCUTBN m_vertexes;
    IndexList         m_indexes;

    Texture const*   m_texture    = nullptr;
    PieceDefinition* m_definition = nullptr;
    Shader*          m_shader     = nullptr;
};
