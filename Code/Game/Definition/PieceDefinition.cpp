//----------------------------------------------------------------------------------------------------
// PieceDefinition.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Definition/PieceDefinition.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Framework/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
STATIC std::vector<PieceDefinition*> PieceDefinition::s_pieceDefinitions;

//----------------------------------------------------------------------------------------------------
PieceDefinition::~PieceDefinition()
{
    for (VertexBuffer* buffer : m_vertexBuffer) SafeDeletePointer(buffer);
    for (IndexBuffer* buffer : m_indexBuffer) SafeDeletePointer(buffer);
}

bool PieceDefinition::LoadFromXmlElement(XmlElement const* element)
{
    m_name            = ParseXmlAttribute(*element, "name", "DEFAULT");
    String const type = ParseXmlAttribute(*element, "type", "DEFAULT");
    if (type == "pawn") m_type = ePieceType::PAWN;
    else if (type == "bishop") m_type = ePieceType::BISHOP;
    else if (type == "knight") m_type = ePieceType::KNIGHT;
    else if (type == "rook") m_type = ePieceType::ROOK;
    else if (type == "queen") m_type = ePieceType::QUEEN;
    else if (type == "king") m_type = ePieceType::KING;

    String const shader         = ParseXmlAttribute(*element, "shader", "DEFAULT");
    m_shader                    = g_theRenderer->CreateOrGetShaderFromFile(shader.c_str(), eVertexType::VERTEX_PCUTBN);
    String const diffuseTexture = ParseXmlAttribute(*element, "diffuseTexture", "DEFAULT");
    m_diffuseTexture            = g_theRenderer->CreateOrGetTextureFromFile(diffuseTexture.c_str());
    String const normalTexture  = ParseXmlAttribute(*element, "normalTexture", "DEFAULT");
    m_normalTexture             = g_theRenderer->CreateOrGetTextureFromFile(normalTexture.c_str());

    XmlElement const* partElement = element->FirstChildElement("PiecePart");

    if (partElement != nullptr)
    {
        while (partElement != nullptr)
        {
            sPiecePart piecePart;
            piecePart.m_name          = ParseXmlAttribute(*partElement, "name", "DEFAULT");
            piecePart.m_startPosition = ParseXmlAttribute(*partElement, "startPosition", Vec3::ZERO);
            piecePart.m_endPosition   = ParseXmlAttribute(*partElement, "endPosition", Vec3::ZERO);
            piecePart.m_orientation   = ParseXmlAttribute(*partElement, "orientation", EulerAngles::ZERO);
            piecePart.m_halfDimension = ParseXmlAttribute(*partElement, "halfDimension", Vec3::ZERO);
            piecePart.m_radius        = ParseXmlAttribute(*partElement, "radius", 0.f);
            m_pieceParts.push_back(piecePart);
            partElement = partElement->NextSiblingElement();
        }
    }

    return true;
}

void PieceDefinition::CreateMeshByID(int const id)
{
    VertexList_PCUTBN verts;
    IndexList         indexes;

    for (auto const& [name, startPosition, endPosition,orientation,halfDimension, radius] : m_pieceParts)
    {
        if (name == "sphere") AddVertsForSphere3D(verts, indexes, startPosition, radius);
        else if (name == "aabb3") AddVertsForAABB3D(verts, indexes, AABB3(startPosition, endPosition));
        else if (name == "cylinder") AddVertsForCylinder3D(verts, indexes, startPosition, endPosition, radius);
        else if (name == "obb3")
        {
            Mat44 matrix = orientation.GetAsMatrix_IFwd_JLeft_KUp();
            AddVertsForOBB3D(verts, indexes, OBB3(startPosition, halfDimension, matrix.GetIBasis3D(), matrix.GetJBasis3D(), matrix.GetKBasis3D()));
        }
    }

    m_vertexBuffer[id] = g_theRenderer->CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
    m_indexBuffer[id]  = g_theRenderer->CreateIndexBuffer(sizeof(unsigned int), sizeof(unsigned int));

    g_theRenderer->CopyCPUToGPU(verts.data(), static_cast<int>(verts.size()) * sizeof(Vertex_PCUTBN), m_vertexBuffer[id]);
    g_theRenderer->CopyCPUToGPU(indexes.data(), static_cast<int>(indexes.size()) * sizeof(unsigned int), m_indexBuffer[id]);
}

//----------------------------------------------------------------------------------------------------
unsigned int PieceDefinition::GetIndexCountByID(int const id) const
{
    return m_indexBuffer[id]->GetSize() / m_indexBuffer[id]->GetStride();
}

void PieceDefinition::InitializeDefs(char const* path)
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

PieceDefinition* PieceDefinition::GetDefByName(String const& name)
{
    for (PieceDefinition* pieceDef : s_pieceDefinitions)
    {
        if (pieceDef->m_name == name)
        {
            return pieceDef;
        }
    }

    return nullptr;
}

void PieceDefinition::ClearAllDefs()
{
    for (PieceDefinition const* pieceDef : s_pieceDefinitions)
    {
        delete pieceDef;
    }

    s_pieceDefinitions.clear();
}
