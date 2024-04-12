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
        return error.length() > 0
            ? std::shared_ptr<const Model3D>(new Model3D(QString::fromStdString(error)))
            : std::shared_ptr<const Model3D>(new Model3D(QString::fromStdString(warn)));

    if (attrib.vertices.empty())
    {
        const auto error = QLatin1String("No vertex data provided in %1").arg(_objFilePath);
        LogPrintf(LogSeverityLevel::Error, qPrintable(error));
        return std::shared_ptr<const Model3D>(new Model3D(error));
    }

    if (materials.empty() && attrib.colors.size() != attrib.vertices.size())
    {
        const auto error = QLatin1String("Incompleted color data provided in %1").arg(_objFilePath);
        LogPrintf(LogSeverityLevel::Error, qPrintable(error));
        return std::shared_ptr<const Model3D>(new Model3D(error));
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
                const auto vertexOrder = mesh.indices[idx].vertex_index;
                if (vertexOrder == -1)
                    break;
                
                const auto vertexIdx = vertexOrder * vertexNumberPerFace;

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
                vertex.xyz[0] = x;
                vertex.xyz[1] = y;
                vertex.xyz[2] = z;
                vertex.materialIndex = materialIdx;
                if (materialIdx == -1)
                {
                    vertex.color = FColorARGB(
                        1.0f,
                        attrib.colors[vertexIdx + 0],
                        attrib.colors[vertexIdx + 1],
                        attrib.colors[vertexIdx + 2]);
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
        const auto error = QLatin1String("3D Model is flat in one of axis");
        LogPrintf(LogSeverityLevel::Error, qPrintable(error));
        return std::shared_ptr<const Model3D>(new Model3D(error));
    }

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
    }

    if (translateToOrigin)
    {
        const auto translateX = -(bbox.minX + bbox.lengthX() / 2.0f);
        const auto translateY = -bbox.minY;
        const auto translateZ = -(bbox.minZ + bbox.lengthZ() / 2.0f);

        for (auto i = 0u; i < vertices.size(); i++)
        {
            auto& vertex = vertices[i];
            vertex.xyz[0] += translateX;
            vertex.xyz[1] += translateY;
            vertex.xyz[2] += translateZ;
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