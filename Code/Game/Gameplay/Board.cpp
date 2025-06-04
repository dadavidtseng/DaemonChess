//----------------------------------------------------------------------------------------------------
// Prop.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Board.hpp"

#include "Piece.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "ThirdParty/stb/stb_image.h"

//----------------------------------------------------------------------------------------------------
Board::Board(Match* owner, Texture const* texture)
    : Actor(owner),
      m_texture(texture)
{
    m_shader = g_theRenderer->CreateOrGetShaderFromFile("Data/Shaders/Diffuse", eVertexType::VERTEX_PCUTBN);
    InitializeLocalVertsForAABB3s();

    for (BoardDefinition* boardDefs : BoardDefinition::s_boardDefinitions)
    {
        for (sSquareInfo squareInfo : boardDefs->m_squareInfos)
        {
            Piece* piece = new Piece(m_match, squareInfo);
            piece->UpdatePositionByCoords(squareInfo.m_coord);
            piece->m_coords = squareInfo.m_coord;
            m_pieceList.push_back(piece);

        }
    }
}

//----------------------------------------------------------------------------------------------------
void Board::Update(float const deltaSeconds)
{
    m_orientation.m_yawDegrees += m_angularVelocity.m_yawDegrees * deltaSeconds;
    m_orientation.m_pitchDegrees += m_angularVelocity.m_pitchDegrees * deltaSeconds;
    m_orientation.m_rollDegrees += m_angularVelocity.m_rollDegrees * deltaSeconds;
}

//----------------------------------------------------------------------------------------------------
void Board::Render() const
{
    g_theRenderer->SetModelConstants(GetModelToWorldTransform(), m_color);
    g_theRenderer->SetBlendMode(eBlendMode::OPAQUE); //AL
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);  //SOLID_CULL_NONE
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);  //DISABLE
    g_theRenderer->BindTexture(m_texture);
    g_theRenderer->BindShader(m_shader);
    // g_theRenderer->DrawVertexArray(static_cast<int>(m_vertexes.size()), m_vertexes.data());
    g_theRenderer->DrawVertexArray(m_vertexes, m_indexes);
}

Vec3 Board::GetWorldPositionByCoords(IntVec2 const& coords)
{
    return Vec3((float)(coords.x) - 0.5f, (float)(coords.y) - 0.5f, 0.2f);
}

Piece* Board::GetPieceByCoords(IntVec2 const& coords)
{
    for (Piece* piece : m_pieceList)
    {
        if (piece->m_coords == coords)
        {
            return piece;
        }
    }
}

IntVec2 Board::StringToChessCoord(String const& chessPos)
{
    if (chessPos.length() != 2) return IntVec2(-1, -1);  // 非法輸入

    char file = tolower(chessPos[0]); // 'a'~'h'
    char rank = chessPos[1];          // '1'~'8'

    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return IntVec2(-1, -1); // 非法座標

    int col = file - 'a' + 1;  // 'a' -> 1, 'b' -> 2, ..., 'h' -> 8
    int row = rank - '0';      // '3' -> 3

    return IntVec2(col, row);
}

void Board::InitializeLocalVertsForAABB3s()
{
    AABB3 bound = AABB3::ZERO_TO_ONE;
    // AddVertsForAABB3D(m_vertexes, m_indexes, bound);

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            Vec3  mins = Vec3(static_cast<float>(x), static_cast<float>(y), 0.f);
            Vec3  maxs = mins + Vec3(1.f, 1.f, 0.2f);
            AABB3 box(mins, maxs);

            bool  isBlack = (x + y) % 2 == 0;
            Rgba8 color   = isBlack ? Rgba8::BLACK : Rgba8::WHITE;

            AddVertsForAABB3D(m_vertexes, m_indexes, box, color);
            // AddVertsForCylinder3D(m_vertexes, m_indexes, mins, mins + Vec3::Z_BASIS, 0.5f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 1024);
            // AddVertsForSphere3D(m_vertexes, m_indexes, mins, 0.5f);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void Board::InitializeLocalVertsForGrid()
{
    float gridLineLength = 100.f;

    for (int i = -(int)gridLineLength / 2; i < (int)gridLineLength / 2; i++)
    {
        float lineWidth = 0.05f;
        if (i == 0) lineWidth = 0.3f;

        AABB3 boundsX = AABB3(Vec3(-gridLineLength / 2.f, -lineWidth / 2.f + (float)i, -lineWidth / 2.f), Vec3(gridLineLength / 2.f, lineWidth / 2.f + (float)i, lineWidth / 2.f));
        AABB3 boundsY = AABB3(Vec3(-lineWidth / 2.f + (float)i, -gridLineLength / 2.f, -lineWidth / 2.f), Vec3(lineWidth / 2.f + (float)i, gridLineLength / 2.f, lineWidth / 2.f));

        Rgba8 colorX = Rgba8::DARK_GREY;
        Rgba8 colorY = Rgba8::DARK_GREY;

        if (i % 5 == 0)
        {
            colorX = Rgba8::RED;
            colorY = Rgba8::GREEN;
        }

        AddVertsForAABB3D(m_vertexes, m_indexes, boundsX, colorX);
        AddVertsForAABB3D(m_vertexes, m_indexes, boundsY, colorY);
    }
}
