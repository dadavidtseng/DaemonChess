//----------------------------------------------------------------------------------------------------
// PieceDefinition.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Definition/PieceDefinition.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
STATIC std::vector<PieceDefinition*> PieceDefinition::s_pieceDefinitions;


//----------------------------------------------------------------------------------------------------
PieceDefinition::~PieceDefinition()
{
    for (PieceDefinition const* pieceDef : s_pieceDefinitions)
    {
        delete pieceDef;
    }

    s_pieceDefinitions.clear();
}

bool PieceDefinition::LoadFromXmlElement(XmlElement const* element)
{
    m_name            = ParseXmlAttribute(*element, "name", "DEFAULT");
    String const type = ParseXmlAttribute(*element, "type", "DEFAULT");
    if (type == "PAWN") m_type = ePieceType::PAWN;
    else if (type == "BISHOP") m_type = ePieceType::BISHOP;
    else if (type == "KNIGHT") m_type = ePieceType::KNIGHT;
    else if (type == "ROOK") m_type = ePieceType::ROOK;
    else if (type == "QUEEN") m_type = ePieceType::QUEEN;
    else if (type == "KING") m_type = ePieceType::KING;

    XmlElement const* partElement = element->FirstChildElement("PiecePart");

    if (partElement != nullptr)
    {
        while (partElement != nullptr)
        {
            sPiecePart piecePart;
            piecePart.m_name          = ParseXmlAttribute(*partElement, "name", "DEFAULT");
            piecePart.m_startPosition = ParseXmlAttribute(*partElement, "startPosition", Vec3::ZERO);
            piecePart.m_endPosition   = ParseXmlAttribute(*partElement, "endPosition", Vec3::ZERO);
            piecePart.m_radius        = ParseXmlAttribute(*partElement, "radius", 0.f);
            piecePart.m_color         = ParseXmlAttribute(*partElement, "color", Rgba8::WHITE);
            m_pieceParts.push_back(piecePart);
            partElement = partElement->NextSiblingElement();
        }
    }

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
