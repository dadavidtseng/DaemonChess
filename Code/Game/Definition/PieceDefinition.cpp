//----------------------------------------------------------------------------------------------------
// PieceDefinition.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Definition/PieceDefinition.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
STATIC std::vector<PieceDefinition*> PieceDefinition::s_pieceDefinitions;


bool PieceDefinition::LoadFromXmlElement(XmlElement const* element)
{
    m_name           = ParseXmlAttribute(*element, "name", "DEFAULT");

    return true;
}

void PieceDefinition::InitializePieceDefs(char const* path)
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

    XmlElement const* pieceDefinitionElement = rootElement->FirstChildElement();

    while (pieceDefinitionElement != nullptr)
    {
        String           elementName     = pieceDefinitionElement->Name();
        PieceDefinition* pieceDefinition = new PieceDefinition();

        if (pieceDefinition->LoadFromXmlElement(pieceDefinitionElement))
        {
            s_pieceDefinitions.push_back(pieceDefinition);
        }
        else
        {
            delete &pieceDefinition;
            ERROR_AND_DIE("Failed to load piece definition")
        }

        pieceDefinitionElement = pieceDefinitionElement->NextSiblingElement();
    }
}
