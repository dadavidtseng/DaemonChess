//----------------------------------------------------------------------------------------------------
// Piece.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

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
    explicit Piece(Match* owner, sSquareInfo const& squareInfo);

    Vec3 CalculateKnightHopPosition(float t);

    void Update(float deltaSeconds) override;
    void Render() const override;
    void RenderSelectedPiece() const;
    void RenderTargetPiece() const;

    void UpdatePositionByCoords(IntVec2 const& newCoords);
    void UpdatePositionByCoords(IntVec2 const& newCoords, float moveTime);
    void StartCaptureAnimation(float duration); // 新增：開始被捕獲動畫


protected:
    Texture*         m_diffuseTexture           = nullptr;
    Texture*         m_normalTexture            = nullptr;
    Texture*         m_specularGlossEmitTexture = nullptr;
    Shader*          m_shader                   = nullptr;
    PieceDefinition* m_definition               = nullptr;
    int              m_id                       = -1;

    bool  m_hasMoved     = false;
    bool  m_isMoving     = false;
    bool  m_isCaptured   = false; // 新增：標記棋子是否被捕獲
    bool  m_isBeingCaptured = false; // 新增：標記棋子正在被捕獲（播放被擊中動畫）
    float m_captureAnimTimer = 0.f;  // 被捕獲動畫計時器
    float m_moveTimer    = 0.f;
    float m_moveDuration = 0.f;
    // Vec3    m_targetPosition = Vec3::ZERO;
    IntVec2 m_startCoords   = IntVec2::ZERO;
    IntVec2 m_targetCoords  = IntVec2::ZERO;
    bool    m_isHighlighted = false;
    bool    m_isSelected    = false;
    // Vec3    m_startPosition  = Vec3::ZERO;
    // IntVec2 m_currentCoords
    // IntVec2 m_prevCoords;
    // float m_secondSinceMOved = 0.f;
    // int m_turnLastMoved = -1;
};
