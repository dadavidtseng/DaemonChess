//----------------------------------------------------------------------------------------------------
// BoardDefinition.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Definition/BoardDefinition.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Game/Framework/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
STATIC std::vector<BoardDefinition*> BoardDefinition::s_boardDefinitions;


//----------------------------------------------------------------------------------------------------
BoardDefinition::~BoardDefinition()
{
}

//----------------------------------------------------------------------------------------------------
bool BoardDefinition::LoadFromXmlElement(XmlElement const* element)
{
    m_pieceOrientation = ParseXmlAttribute(*element, "orientation", EulerAngles::ZERO);
    m_pieceColor       = ParseXmlAttribute(*element, "color", Rgba8::WHITE);

    XmlElement const* boardElement = element->FirstChildElement("SquareInfo");

    if (boardElement != nullptr)
    {
        while (boardElement != nullptr)
        {
            sSquareInfo squareInfo;
            squareInfo.m_name               = ParseXmlAttribute(*boardElement, "name", "DEFAULT");
            squareInfo.m_notation           = ParseXmlAttribute(*boardElement, "notation", "DEFAULT");
            squareInfo.m_playerControllerId = ParseXmlAttribute(*boardElement, "id", -1);
            squareInfo.m_coords             = ParseXmlAttribute(*boardElement, "coord", IntVec2::ZERO);
            m_squareInfos.push_back(squareInfo);
            boardElement = boardElement->NextSiblingElement();
        }
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
void BoardDefinition::InitializeDefs(char const* path)
{
    XmlDocument     document;
    XmlResult const result = document.LoadFile(path);

    if (result != XmlResult::XML_SUCCESS)
    {
        ERROR_AND_DIE("Failed to load XML file")
    }

    XmlElement const* rootElement = document.RootElement();

    if (rootElement == nullptr)
    {
        ERROR_AND_DIE("XML file %s is missing a root element.")
    }

    XmlElement const* boardDefinitionElement = rootElement->FirstChildElement();

    while (boardDefinitionElement != nullptr)
    {
        String           elementName     = boardDefinitionElement->Name();
        BoardDefinition* boardDefinition = new BoardDefinition();

        if (boardDefinition->LoadFromXmlElement(boardDefinitionElement))
        {
            s_boardDefinitions.push_back(boardDefinition);
        }
        else
        {
            delete &boardDefinition;
            ERROR_AND_DIE("Failed to load piece definition")
        }

        boardDefinitionElement = boardDefinitionElement->NextSiblingElement();
    }
}

//----------------------------------------------------------------------------------------------------
STATIC void BoardDefinition::ClearAllDefs()
{
    for (BoardDefinition const* boardDef : s_boardDefinitions)
    {
        GAME_SAFE_RELEASE(boardDef);
    }

    s_boardDefinitions.clear();
}
