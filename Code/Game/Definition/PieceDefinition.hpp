//----------------------------------------------------------------------------------------------------
// PieceDefinition.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"

class Shader;

//----------------------------------------------------------------------------------------------------
enum class ePieceType : int8_t
{
    NONE = -1,
    PAWN,
    BISHOP,
    KNIGHT,
    ROOK,
    QUEEN,
    KING
};

//----------------------------------------------------------------------------------------------------
struct sPiecePart
{
    String      m_name          = "DEFAULT";
    Vec3        m_startPosition = Vec3::ZERO;
    Vec3        m_endPosition   = Vec3::ZERO;
    EulerAngles m_orientation   = EulerAngles::ZERO;
    Vec3        m_halfDimension = Vec3::ZERO;
    float       m_radius        = 0.f;
};

//----------------------------------------------------------------------------------------------------
struct PieceDefinition
{
    PieceDefinition() = default;
    ~PieceDefinition();

    bool         LoadFromXmlElement(XmlElement const* element);
    void         CreateMeshByID(int id);
    unsigned int GetIndexCountByID(int id) const;

    static void                          InitializeDefs(char const* path);
    static PieceDefinition*              GetDefByName(String const& name);
    static std::vector<PieceDefinition*> s_pieceDefinitions;
    static void                          ClearAllDefs();

    String                  m_name           = "DEFAULT";
    ePieceType              m_type           = ePieceType::NONE;
    Shader*                 m_shader         = nullptr;
    Texture*                m_diffuseTexture = nullptr;
    Texture*                m_normalTexture  = nullptr;
    std::vector<sPiecePart> m_pieceParts;
    char                    m_glyph           = '?';
    VertexBuffer*           m_vertexBuffer[2] = {};
    IndexBuffer*            m_indexBuffer[2]  = {};
};
