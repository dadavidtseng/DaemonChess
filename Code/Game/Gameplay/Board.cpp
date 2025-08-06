//----------------------------------------------------------------------------------------------------
// Board.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Gameplay/Board.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/Resource/ModelResource.hpp"
#include "Engine/Resource/ResourceLoader/ObjModelLoader.hpp"
#include "Engine/Resource/ResourceHandle.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Game/Definition/BoardDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Match.hpp"
#include "Game/Gameplay/Piece.hpp"

//----------------------------------------------------------------------------------------------------
Board::Board(Match* owner)
    : Actor(owner)

{
    m_shader                   = g_theRenderer->CreateOrGetShaderFromFile("Data/Shaders/Bloom", eVertexType::VERTEX_PCU);
    m_diffuseTexture           = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PhongTextures/FunkyBricks_d.png");
    m_normalTexture            = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PhongTextures/FunkyBricks_n.png");
    m_specularGlossEmitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PhongTextures/FunkyBricks_sge.png");
    CreateLocalVertsForAABB3s();
    CreateLocalVertsForBoardFrame();

    m_resourceHandle = g_theResourceSubsystem->LoadResource<ModelResource>("Data/Models/TutorialBox_Phong/Tutorial_Box.obj");

    ModelResource const* modelResource = m_resourceHandle.Get();

    // 取得頂點和索引資料
    m_vertexWoman = modelResource->GetVertices();
    m_indexWoman  = modelResource->GetIndices();
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

    if (g_theInput->IsKeyDown(KEYCODE_I)) m_testPos.y++;
    if (g_theInput->IsKeyDown(KEYCODE_J)) m_testPos.x--;
    if (g_theInput->IsKeyDown(KEYCODE_K)) m_testPos.y--;
    if (g_theInput->IsKeyDown(KEYCODE_L)) m_testPos.x++;
}

AABB3 Board::GetAABB3FromCoords(IntVec2 const& coords,
                                float const    aabb3Height) const
{
    // Convert board coordinates to world position
    Vec3 worldPosition = Vec3(
        (float)coords.x - 1.f,
        (float)coords.y - 1.f,
        0.f  // Assuming board is at z=0
    );

    // Create AABB with standard tile size (assuming 1x1x1 tiles)
    Vec3 mins = worldPosition;
    Vec3 maxs = worldPosition + Vec3(1.f, 1.f, aabb3Height);
    return AABB3(mins, maxs);
}

void Board::RenderSelectedBox() const
{
    VertexList_PCU verts;

    for (sSquareInfo const& info : m_squareInfoList)
    {
        if (info.m_isSelected || info.m_isHighlighted)
        {
            AddVertsForWireframeAABB3D(verts, GetAABB3FromCoords(info.m_coords, 0.2f), 0.01f);
        }
    }

    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->BindShader(g_theRenderer->CreateOrGetShaderFromFile("Data/Shaders/Default"));
    g_theRenderer->DrawVertexArray(verts);
}

//----------------------------------------------------------------------------------------------------
void Board::Render() const
{
    g_theRenderer->SetModelConstants(GetModelToWorldTransform(), m_color);
    g_theRenderer->SetBlendMode(eBlendMode::OPAQUE);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(m_diffuseTexture, 0);
    g_theRenderer->BindTexture(m_normalTexture, 1);
    g_theRenderer->BindTexture(m_specularGlossEmitTexture, 2);
    g_theRenderer->BindShader(m_shader);
    g_theRenderer->DrawVertexArray(m_vertexes, m_indexes);

    RenderSelectedBox();

    // Mat44 m2w;
    // m2w.SetTranslation3D(m_testPos);
    // m2w.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());
    // g_theRenderer->SetModelConstants(m2w);
    // g_theRenderer->SetBlendMode(eBlendMode::OPAQUE);
    // g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
    // g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    // g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
    // g_theRenderer->BindTexture(g_theRenderer->CreateOrGetTextureFromFile("Data/Models/Woman/Woman_Diffuse.png"), 0);
    // g_theRenderer->BindTexture(g_theRenderer->CreateOrGetTextureFromFile("Data/Models/Woman/Woman_Normal.png"), 1);
    // g_theRenderer->BindShader(m_shader);
    // g_theRenderer->DrawVertexArray(m_vertexWoman, m_indexWoman);


    if (m_resourceHandle.IsValid())
    {


        // 渲染程式碼保持不變
        Mat44 m2w;
        m2w.SetTranslation3D(m_testPos);
        m2w.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());
        m2w.AppendXRotation(90.f);
        m2w.AppendYRotation(45.f);
        m2w.AppendScaleUniform3D(0.01f);
        g_theRenderer->SetModelConstants(m2w);
        g_theRenderer->SetBlendMode(eBlendMode::OPAQUE);
        g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
        g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
        g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
        g_theRenderer->BindTexture(g_theRenderer->CreateOrGetTextureFromFile("Data/Models/TutorialBox_Phong/Tutorial_Box_Diffuse.tga"), 0);
        g_theRenderer->BindTexture(g_theRenderer->CreateOrGetTextureFromFile("Data/Models/TutorialBox_Phong/Tutorial_Box_Normal.tga"), 1);
        g_theRenderer->BindTexture(g_theRenderer->CreateOrGetTextureFromFile("Data/Models/TutorialBox_Phong/Tutorial_Box_SpecGlossEmit.tga"), 2);
        g_theRenderer->BindShader(m_shader);
        g_theRenderer->DrawVertexArray(m_vertexWoman, m_indexWoman);
    }
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
    sSquareInfo returnSquareInfo;
    for (sSquareInfo squareInfo : m_squareInfoList)
    {
        if (squareInfo.m_coords == coords)
        {
            return squareInfo;
        }
    }
    return returnSquareInfo;
}

IntVec2 Board::StringToChessCoord(String const& chessPos)
{
    if (chessPos.length() != 2) return IntVec2(-1, -1);  // 非法輸入

    int  fileInt = tolower(static_cast<unsigned char>(chessPos[0])); // 'a'~'h'
    char rank    = chessPos[1]; // '1'~'8'

    if (fileInt < 'a' || fileInt > 'h' || rank < '1' || rank > '8') return IntVec2(-1, -1); // 非法座標

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

String Board::GetBoardContents(int const rowNum) const
{
    int constexpr colNum = 8;
    String        result;

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

void Board::CreateLocalVertsForAABB3s()
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
            m_AABBs.push_back(box);
            AddVertsForAABB3D(m_vertexes, m_indexes, box, color);
        }
    }
    // AddVertsForQuad3D(m_vertexes, m_indexes, Vec3(0,0,2), Vec3(1,0,2),Vec3(0,1,2), Vec3(1,1,2));
    // AddVertsForQuad3D(m_vertexes, m_indexes,  Vec3(0,1,3),Vec3(0,0,3), Vec3(1,1,3),Vec3(1,0,3));
    // AddVertsForAABB3D(m_vertexes, m_indexes, AABB3::ZERO_TO_ONE, Rgba8(240, 230, 210));
}

void Board::CreateLocalVertsForBoardFrame()
{
    float constexpr boardSize      = 8.f;
    float constexpr halfSize       = boardSize * 0.5f; // = 4.0f
    float constexpr frameThickness = 0.2f;
    float constexpr frameHeight    = 0.5f;

    // 棋盤中心點在 (4,4)
    float constexpr centerX = 4.f;
    float constexpr centerY = 4.f;

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

void Board::UpdateSquareInfoList(IntVec2 const& toCoords)
{
    sSquareInfo const fromInfo = GetSquareInfoByCoords(toCoords);

    for (auto it = m_squareInfoList.begin(); it != m_squareInfoList.end(); ++it)
    {
        if (it->m_coords == toCoords)
        {
            it->m_name               = "DEFAULT";
            it->m_notation           = "*";
            it->m_playerControllerId = -1;
        }
    }
}

void Board::UpdateSquareInfoList(IntVec2 const& fromCoords,
                                 IntVec2 const& toCoords)
{
    sSquareInfo const fromInfo = GetSquareInfoByCoords(fromCoords);

    for (auto it = m_squareInfoList.begin(); it != m_squareInfoList.end(); ++it)
    {
        if (it->m_coords == toCoords)
        {
            it->m_name               = fromInfo.m_name;
            it->m_notation           = fromInfo.m_notation;
            it->m_playerControllerId = fromInfo.m_playerControllerId;
        }
        else if (it->m_coords == fromCoords)
        {
            it->m_name               = "DEFAULT";
            it->m_notation           = "*";
            it->m_playerControllerId = -1;
        }
    }
}

void Board::UpdateSquareInfoList(IntVec2 const& fromCoords,
                                 IntVec2 const& toCoords,
                                 String const&  promoteTo)
{
    sSquareInfo const fromInfo = GetSquareInfoByCoords(fromCoords);

    for (auto it = m_squareInfoList.begin(); it != m_squareInfoList.end(); ++it)
    {
        if (it->m_coords == toCoords)
        {
            it->m_name               = promoteTo;
            it->m_notation           = 'N';
            it->m_playerControllerId = fromInfo.m_playerControllerId;
        }
        else if (it->m_coords == fromCoords)
        {
            it->m_name               = "DEFAULT";
            it->m_notation           = "*";
            it->m_playerControllerId = -1;
        }
    }
}

//----------------------------------------------------------------------------------------------------
/// @brief Finds the coordinates of the king in `m_squareInfoList` belonging to the specified player.
/// @param playerId The ID of the player whose king's position is being queried.
/// @return Coordinates of the king; returns IntVec2::NEGATIVE_ONE if not found.
IntVec2 Board::FindKingCoordsByPlayerId(int const playerId) const
{
    for (sSquareInfo const& squareInfo : m_squareInfoList)
    {
        if (squareInfo.m_playerControllerId == playerId && squareInfo.m_name == "king")
        {
            return squareInfo.m_coords;
        }
    }

    ERROR_RECOVERABLE(Stringf("King not found for player ID %d in m_squareInfoList.", playerId))
    return IntVec2::NEGATIVE_ONE;
}
