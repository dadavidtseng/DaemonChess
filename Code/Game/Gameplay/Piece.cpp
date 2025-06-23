//----------------------------------------------------------------------------------------------------
// Piece.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Piece.hpp"

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Definition/PieceDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Match.hpp"
#include "ThirdParty/stb/stb_image.h"

//----------------------------------------------------------------------------------------------------
Piece::Piece(Match* owner, sSquareInfo const& squareInfo)
    : Actor(owner)
{
    m_definition = PieceDefinition::GetDefByName(squareInfo.m_name);

    m_shader         = m_definition->m_shader;
    m_diffuseTexture = m_definition->m_diffuseTexture;
    m_normalTexture  = m_definition->m_normalTexture;
    m_coords         = squareInfo.m_coords;
    m_id             = squareInfo.m_playerControllerId;

    UpdatePositionByCoords(squareInfo.m_coords);
    // m_orientation = EulerAngles(45,0,0);
}


//----------------------------------------------------------------------------------------------------
void Piece::Update(float const deltaSeconds)
{
    // Interpolation finishes no matter the value of FPS.
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
    g_theRenderer->BindTexture(m_diffuseTexture, 0);
    g_theRenderer->BindTexture(m_normalTexture, 1);
    g_theRenderer->BindShader(m_shader);
    unsigned int const indexCount = m_definition->GetIndexCountByID(m_id);
    g_theRenderer->DrawIndexedVertexBuffer(m_definition->m_vertexBuffer[m_id], m_definition->m_indexBuffer[m_id], indexCount);
}

void Piece::UpdatePositionByCoords(IntVec2 const& newCoords)
{
    m_position = m_match->m_board->GetWorldPositionByCoords(newCoords);
    m_coords   = newCoords;
}
