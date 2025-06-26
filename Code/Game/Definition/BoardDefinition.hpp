//----------------------------------------------------------------------------------------------------
// BoardDefinition.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/IntVec2.hpp"

struct sSquareInfo
{
    String  m_name               = "DEFAULT";
    String  m_notation           = "DEFAULT";
    int     m_playerControllerId = -1;
    Rgba8   m_color              = Rgba8::WHITE;
    IntVec2 m_coords             = IntVec2::ZERO;
    bool    m_isHighlighted      = false;
    bool    m_isSelected         = false;
};


//----------------------------------------------------------------------------------------------------
struct BoardDefinition
{
    BoardDefinition() = default;
    ~BoardDefinition();

    bool LoadFromXmlElement(XmlElement const* element);

    static void                          InitializeDefs(char const* path);
    static std::vector<BoardDefinition*> s_boardDefinitions;
    static void                          ClearAllDefs();

    std::vector<sSquareInfo> m_squareInfos;
    EulerAngles              m_pieceOrientation = EulerAngles::ZERO;
    Rgba8                    m_pieceColor       = Rgba8::WHITE;
};
