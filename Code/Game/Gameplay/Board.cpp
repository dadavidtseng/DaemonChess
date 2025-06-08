//----------------------------------------------------------------------------------------------------
// Board.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Board.hpp"

#include "Match.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Piece.hpp"
#include "ThirdParty/stb/stb_image.h"

//----------------------------------------------------------------------------------------------------
Board::Board(Match* owner, Texture const* texture)
    : Actor(owner),
      m_texture(texture)
{
    m_shader = g_theRenderer->CreateOrGetShaderFromFile("Data/Shaders/Diffuse", eVertexType::VERTEX_PCUTBN);
    InitializeLocalVertsForAABB3s();
    InitializeLocalVertsForBoardFrame();


}

Board::~Board()
{
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
    g_theRenderer->SetBlendMode(eBlendMode::OPAQUE);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(m_texture);
    g_theRenderer->BindShader(m_shader);
    g_theRenderer->DrawVertexArray(m_vertexes, m_indexes);
}

//----------------------------------------------------------------------------------------------------
Vec3 Board::GetWorldPositionByCoords(IntVec2 const& coords)
{
    return Vec3(static_cast<float>(coords.x) - 0.5f, static_cast<float>(coords.y) - 0.5f, 0.2f);
}

//----------------------------------------------------------------------------------------------------
Piece* Board::GetPieceByCoords(IntVec2 const& coords) const
{
    for (Piece* piece : m_match->m_pieceList)
    {
        if (piece == nullptr)
        {
            continue;
        }
        if (piece->m_coords == coords)
        {
            return piece;
        }
    }

    return nullptr;
}

sSquareInfo Board::GetSquareInfoByCoords(IntVec2 const& coords)
{
    for (sSquareInfo squareInfo : m_squareInfoList)
    {
        if (squareInfo.m_coords == coords)
        {
            return squareInfo;
        }
    }
    return sSquareInfo{};
}

IntVec2 Board::StringToChessCoord(String const& chessPos)
{
    if (chessPos.length() != 2) return IntVec2(-1, -1);  // 非法輸入

    int fileInt = tolower(static_cast<unsigned char>(chessPos[0])); // 'a'~'h'
    char rank = chessPos[1]; // '1'~'8'

    if (fileInt < 'a' || fileInt > 'h' || rank < '1' || rank > '8')
        return IntVec2(-1, -1); // 非法座標

    int col = fileInt - 'a' + 1;  // 'a' -> 1, ..., 'h' -> 8
    int row = rank - '0';         // '3' -> 3

    return IntVec2(col, row);
}

String Board::ChessCoordToString(IntVec2 const& coords)
{
    int col = coords.x;  // 假設 IntVec2.x 是欄（1~8）
    int row = coords.y;  // IntVec2.y 是列（1~8）

    // 非法檢查，若超出棋盤範圍，回傳空字串
    if (col < 1 || col > 8 || row < 1 || row > 8) return String("");

    char file = static_cast<char>('a' + (col - 1));  // 1 -> 'a', ..., 8 -> 'h'
    char rank = static_cast<char>('0' + row);        // 1 -> '1', ..., 8 -> '8'

    String result;
    result += file;
    result += rank;

    return result;
}

String Board::GetBoardContents(int rowNum)
{
    const int colNum = 8;
    String    result;

    // rowNum: 1 ~ 8（最上到最下）
    int startIndex = (rowNum - 1) * colNum;
    for (int i = 0; i < colNum; ++i)
    {
        result += m_squareInfoList[startIndex + i].m_notation;
    }

    return result;
}

bool Board::IsCoordValid(IntVec2 const& coords) const
{
    return coords.x >= 1 && coords.x <= 8 && coords.y >= 1 && coords.y <= 8;
}

void Board::InitializeLocalVertsForAABB3s()
{
    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            Vec3  mins = Vec3(static_cast<float>(x), static_cast<float>(y), 0.f);
            Vec3  maxs = mins + Vec3(1.f, 1.f, 0.2f);
            AABB3 box  = AABB3(mins, maxs);

            bool const isBlack = (x + y) % 2 == 0;
            Rgba8      color   = isBlack ? Rgba8(40, 50, 60) : Rgba8(240, 230, 210);

            AddVertsForAABB3D(m_vertexes, m_indexes, box, color);
        }
    }
}

void Board::InitializeLocalVertsForBoardFrame()
{
    constexpr float boardSize      = 8.0f;
    constexpr float halfSize       = boardSize * 0.5f; // = 4.0f
    constexpr float frameThickness = 0.2f;
    constexpr float frameHeight    = 0.5f;

    // 棋盤中心點在 (4,4)
    const float centerX = 4.0f;
    const float centerY = 4.0f;

    // Bottom Frame
    AABB3 bottomFrame = AABB3(
        Vec3(centerX - halfSize - frameThickness, centerY - halfSize - frameThickness, 0.f),
        Vec3(centerX + halfSize + frameThickness, centerY - halfSize, frameHeight)
    );

    // Top Frame
    AABB3 topFrame = AABB3(
        Vec3(centerX - halfSize - frameThickness, centerY + halfSize, 0.f),
        Vec3(centerX + halfSize + frameThickness, centerY + halfSize + frameThickness, frameHeight)
    );

    // Left Frame
    AABB3 leftFrame = AABB3(
        Vec3(centerX - halfSize - frameThickness, centerY - halfSize, 0.f),
        Vec3(centerX - halfSize, centerY + halfSize, frameHeight)
    );

    // Right Frame
    AABB3 rightFrame = AABB3(
        Vec3(centerX + halfSize, centerY - halfSize, 0.f),
        Vec3(centerX + halfSize + frameThickness, centerY + halfSize, frameHeight)
    );

    AddVertsForAABB3D(m_vertexes, m_indexes, bottomFrame, Rgba8(40, 50, 60));
    AddVertsForAABB3D(m_vertexes, m_indexes, topFrame, Rgba8(40, 50, 60));
    AddVertsForAABB3D(m_vertexes, m_indexes, leftFrame, Rgba8(40, 50, 60));
    AddVertsForAABB3D(m_vertexes, m_indexes, rightFrame, Rgba8(40, 50, 60));
}

void Board::UpdateBoardSquareInfoList(IntVec2 const& fromCoords, IntVec2 const& toCoords)
{
    sSquareInfo const fromInfo = GetSquareInfoByCoords(fromCoords);

    for (auto it = m_squareInfoList.begin(); it != m_squareInfoList.end(); ++it)
    {
        if (it->m_coords == toCoords)
        {
            *it = fromInfo; // 用暫存的，不怕被修改掉
        }
        else if (it->m_coords == fromCoords)
        {
            it->m_name               = "DEFAULT";
            it->m_notation           = "*";
            it->m_playerControllerId = -1;
        }
    }
}

void Board::CapturePiece(IntVec2 const& fromCoords,
                         IntVec2 const& toCoords)
{
    // UpdateBoardSquareInfoList(fromCoords, toCoords);
    m_match->UpdatePieceList(fromCoords, toCoords);
}
