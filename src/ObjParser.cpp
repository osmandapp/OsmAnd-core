#include "ObjParser.h"

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"
#endif // !defined(TINYOBJLOADER_IMPLEMENTATION)

#include "Logging.h"
#include "Model3D.h"

#include <QVector>

OsmAnd::ObjParser::ObjParser(const QString& objFilePath, const QString& mtlDirPath)
    : _objFilePath(objFilePath)
    , _mtlDirPath(mtlDirPath)
{
}

OsmAnd::ObjParser::~ObjParser()
{
}

std::shared_ptr<const OsmAnd::Model3D> OsmAnd::ObjParser::parse(const bool translateToOrigin /*= true*/) const
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn = "";
    std::string error = "";

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &error, _objFilePath.toLatin1().data(), _mtlDirPath.toLatin1().data(), true, false);

    if (warn.length() > 0)
        LogPrintf(LogSeverityLevel::Warning, warn.c_str());

    if (error.length() > 0)
        LogPrintf(LogSeverityLevel::Error, error.c_str());

    if (!ok)
        return nullptr;

    if (attrib.vertices.empty())
    {
        LogPrintf(LogSeverityLevel::Error, "No vertex data provided for 3D model at '%s'", qPrintable(_objFilePath));
        return nullptr;
    }

    if (materials.empty() && attrib.colors.size() != attrib.vertices.size())
    {
        LogPrintf(LogSeverityLevel::Error, "Incomplete color data provided for 3D Model at '%s'", qPrintable(_objFilePath));
        return nullptr;
    }

    int dummyMaterialIdx = -1;
    
    QVector<Model3D::Material> processedMaterials(materials.size());
    for (int i = 0; i < materials.size(); i++)
    {
        const auto& material = materials[i];

        Model3D::Material processedMaterial;
        processedMaterial.color = FColorARGB(
            material.dissolve,
            material.diffuse[0],
            material.diffuse[1],
            material.diffuse[2]);
        processedMaterial.name = QString::fromStdString(material.name);
        processedMaterials[i] = processedMaterial;
        if (processedMaterial.name.contains("profile_color"))
            dummyMaterialIdx = i;
    }

    QVector<Model3D::Vertex> vertices;

    Model3D::BBox bbox;
    bbox.minX = attrib.vertices[0];
    bbox.maxX = bbox.minX;
    bbox.minY = attrib.vertices[1];
    bbox.maxY = bbox.minY;
    bbox.minZ = attrib.vertices[2];
    bbox.maxZ = bbox.minZ;

    for (const auto& shape : shapes)
    {
        const auto& mesh = shape.mesh;
        for (auto faceIdx = 0u; faceIdx < mesh.num_face_vertices.size(); faceIdx++)
        {
            const auto vertexNumberPerFace = mesh.num_face_vertices[faceIdx];
            assert(vertexNumberPerFace == 3); // Should be triangulated by tinyobj

            const auto materialIdx = mesh.material_ids[faceIdx];

            // Indices to vector of vertex indices
            const auto startVertexIdx = faceIdx * vertexNumberPerFace;
            const auto endVertexIdx = startVertexIdx + vertexNumberPerFace;

            QVector<Model3D::Vertex> faceVertices;

            for (auto idx = startVertexIdx; idx < endVertexIdx; idx++)
            {
                const auto vertexIdx = mesh.indices[idx].vertex_index * 3;
                const auto normalIdx = mesh.indices[idx].normal_index * 3;
                if (vertexIdx < 0 || normalIdx < 0)
                    break;
                
                const float x = attrib.vertices[vertexIdx + 0];
                const float y = attrib.vertices[vertexIdx + 1];
                const float z = attrib.vertices[vertexIdx + 2];

                // Update min/max values 
                if (x < bbox.minX)
                    bbox.minX = x;
                if (x > bbox.maxX)
                    bbox.maxX = x;
                if (y < bbox.minY)
                    bbox.minY = y;
                if (y > bbox.maxY)
                    bbox.maxY = y;
                if (z < bbox.minZ)
                    bbox.minZ = z;
                if (z > bbox.maxZ)
                    bbox.maxZ = z;

                Model3D::Vertex vertex;
                vertex.position[0] = x;
                vertex.position[1] = y;
                vertex.position[2] = z;
                vertex.normal[0] = attrib.normals[normalIdx + 0];
                vertex.normal[1] = attrib.normals[normalIdx + 1];
                vertex.normal[2] = attrib.normals[normalIdx + 2];
                
                vertex.materialIndex = materialIdx;
                if (materialIdx == -1)
                {
                    vertex.color = FColorARGB(
                        1.0f,
                        attrib.colors[vertexIdx + 0],
                        attrib.colors[vertexIdx + 1],
                        attrib.colors[vertexIdx + 2]);
                }
                else if (materialIdx == dummyMaterialIdx)
                {
                    vertex.materialIndex = -1;
                    vertex.color = FColorARGB(NAN, NAN, NAN, NAN);
                }

                faceVertices.push_back(vertex);
            }

            if (faceVertices.length() == vertexNumberPerFace)
                for (const auto& vertex : faceVertices)
                    vertices.push_back(vertex);
        }
    }

    if (qFuzzyIsNull(bbox.lengthX()) || qFuzzyIsNull(bbox.lengthY()) || qFuzzyIsNull(bbox.lengthZ()))
    {
        LogPrintf(LogSeverityLevel::Error, "3D Model at '%s' is flat in one of axis", qPrintable(_objFilePath));
        return nullptr;
    }

    if (translateToOrigin)
    {
        const auto translateX = -(bbox.minX + bbox.lengthX() / 2.0f);
        const auto translateY = -bbox.minY;
        const auto translateZ = -(bbox.minZ + bbox.lengthZ() / 2.0f);

        for (auto i = 0u; i < vertices.size(); i++)
        {
            auto& vertex = vertices[i];
            vertex.position[0] += translateX;
            vertex.position[1] += translateY;
            vertex.position[2] += translateZ;
        }

        bbox.minX += translateX;
        bbox.maxX += translateX;
        bbox.minY += translateY;
        bbox.maxY += translateY;
        bbox.minZ += translateZ;
        bbox.maxZ += translateZ;
    }

    return std::shared_ptr<const Model3D>(new Model3D(vertices, processedMaterials, bbox));
}