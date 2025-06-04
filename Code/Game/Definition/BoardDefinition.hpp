//----------------------------------------------------------------------------------------------------
// BoardDefinition.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "PieceDefinition.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"

struct sSquareInfo
{
    String  m_name     = "DEFAULT";
    String  m_notation = "DEFAULT";
    IntVec2 m_coord    = IntVec2::ZERO;
};


//----------------------------------------------------------------------------------------------------
struct BoardDefinition
{
    BoardDefinition() = default;
    ~BoardDefinition();

    bool LoadFromXmlElement(XmlElement const* element);

    static void                          InitializeDefs(char const* path);
    static std::vector<BoardDefinition*> s_boardDefinitions;

    std::vector<sSquareInfo> m_squareInfos;
};
