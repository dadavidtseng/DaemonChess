//----------------------------------------------------------------------------------------------------
// Piece.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

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
    friend class Match;

public:
    explicit Piece(Match* owner, sSquareInfo const& squareInfo, Texture* texture = nullptr);

    void Update(float deltaSeconds) override;
    void Render() const override;

    void UpdatePositionByCoords(IntVec2 const& newCoords);

protected:
    Texture*         m_texture    = nullptr;
    Shader*          m_shader     = nullptr;
    PieceDefinition* m_definition = nullptr;
    int              m_id         = -1;
};
