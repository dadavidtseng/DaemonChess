//----------------------------------------------------------------------------------------------------
// PieceDefinition.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Core/StringUtils.hpp"
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
class PieceDefinition
{
public:
    void CreateMeshForEachPlayer(int playerIndex);

    ePieceType    m_type            = ePieceType::NONE;
    String        m_name            = "DEFAULT";
    char          m_glyph           = '?';
    IndexBuffer*  m_indexBuffer[2]  = {};
    VertexBuffer* m_vertexBuffer[2] = {};
};
