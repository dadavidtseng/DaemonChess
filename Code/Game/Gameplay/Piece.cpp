//----------------------------------------------------------------------------------------------------
// Piece.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Piece.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Match.hpp"
#include "ThirdParty/stb/stb_image.h"

// TODO: AddVertsForOBB3D();
//----------------------------------------------------------------------------------------------------
Piece::Piece(Match* owner, sSquareInfo const& squareInfo, Texture const* texture)
    : Actor(owner),
      m_texture(texture)
{
    m_definition = PieceDefinition::GetDefByName(squareInfo.m_name);
    m_shader     = m_definition->m_shader;

    for (auto const& [name, startPosition, endPosition, radius, color] : m_definition->m_pieceParts)
    {
        if (name == "sphere") AddVertsForSphere3D(m_vertexes, m_indexes, startPosition, radius, color);
        else if (name == "aabb3") AddVertsForAABB3D(m_vertexes, m_indexes, AABB3(startPosition, endPosition), color);
        else if (name == "cylinder") AddVertsForCylinder3D(m_vertexes, m_indexes, startPosition, endPosition, radius, color);
    }
}

//----------------------------------------------------------------------------------------------------
void Piece::Update(float const deltaSeconds)
{
    m_orientation.m_yawDegrees += m_angularVelocity.m_yawDegrees * deltaSeconds;
    m_orientation.m_pitchDegrees += m_angularVelocity.m_pitchDegrees * deltaSeconds;
    m_orientation.m_rollDegrees += m_angularVelocity.m_rollDegrees * deltaSeconds;
}

//----------------------------------------------------------------------------------------------------
void Piece::Render() const
{
    g_theRenderer->SetModelConstants(GetModelToWorldTransform(), m_color);
    g_theRenderer->SetBlendMode(eBlendMode::OPAQUE);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(m_texture);
    g_theRenderer->BindShader(m_shader);
    g_theRenderer->DrawVertexArray(m_vertexes, m_indexes);
}

void Piece::UpdatePositionByCoords(IntVec2 const& newCoords)
{
    m_position = m_match->m_board->GetWorldPositionByCoords(newCoords);
}
