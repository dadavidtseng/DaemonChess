//----------------------------------------------------------------------------------------------------
// PieceDefinition.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"

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
struct PieceDefinition
{
    PieceDefinition() = default;
    ~PieceDefinition();

    bool LoadFromXmlElement(XmlElement const* element);
    void CreateMeshForEachPlayer(int playerIndex);

    static void InitializePieceDefs(char const* path);
    static std::vector<PieceDefinition*> s_pieceDefinitions;

    ePieceType    m_type            = ePieceType::NONE;
    String        m_name            = "DEFAULT";
    char          m_glyph           = '?';
    IndexBuffer*  m_indexBuffer[2]  = {};
    VertexBuffer* m_vertexBuffer[2] = {};
};
