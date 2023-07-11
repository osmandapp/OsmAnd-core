#include "ObjParser.h"

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"
#endif // !defined(TINYOBJLOADER_IMPLEMENTATION)

#include "Logging.h"
#include "Model3D.h"

#include <QFile>
#include <QFileInfo>

OsmAnd::ObjParser::ObjParser(const QString& objFilename, const QString& mtlDirPath)
    : _objFilename(objFilename)
    , _mtlDirPath(mtlDirPath)
{
}

OsmAnd::ObjParser::~ObjParser()
{
}

bool OsmAnd::ObjParser::parse(std::shared_ptr<Model3D>& outModel, const bool translateToOrigin) const
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn = "";
    std::string error = "";

    QByteArray objArray = _objFilename.toLatin1();
    char* objFilename = objArray.data();
    QByteArray mtlArray = _mtlDirPath.toLatin1();
    char* mtlDirPath = mtlArray.data();

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &error, objFilename, mtlDirPath, true, false);

    if (warn.length() > 0)
        LogPrintf(LogSeverityLevel::Warning, warn.c_str());

    if (error.length() > 0)
        LogPrintf(LogSeverityLevel::Error, error.c_str());

    if (!ok)
        return false;

    if (attrib.vertices.empty())
    {
        QFileInfo objFileInfo(_objFilename);
        LogPrintf(LogSeverityLevel::Error, "No vertices data provided in %s", qPrintable(objFileInfo.fileName()));
        return false;
    }

    if (materials.empty() && attrib.colors.size() != attrib.vertices.size())
    {
        QFileInfo objFileInfo(_objFilename);
        LogPrintf(LogSeverityLevel::Error, "No color data provided in %s", qPrintable(objFileInfo.fileName()));
        return false;
    }

    QVector<Model3D::VertexInfo> vertices;
    float minX = attrib.vertices[0];
    float maxX = minX;
    float minY = attrib.vertices[1];
    float maxY = minY;
    float minZ = attrib.vertices[2];
    float maxZ = minZ;

    for (const auto& shape : shapes)
    {
        const auto& mesh = shape.mesh;
        for (int faceIdx = 0; faceIdx < mesh.num_face_vertices.size(); faceIdx++)
        {
            const auto vertexNumberPerFace = mesh.num_face_vertices[faceIdx];
            assert(vertexNumberPerFace == 3); // Should be triangulated by tinyobj

            const int materialIdx = mesh.material_ids[faceIdx];

            // Indices to vector of vertex indices
            const int startVertexIdx = faceIdx * vertexNumberPerFace;
            const int endVertexIdx = startVertexIdx + vertexNumberPerFace;

            QVector<Model3D::VertexInfo> faceVertices;

            for (int idx = startVertexIdx; idx < endVertexIdx; idx++)
            {
                int vertexOrder = mesh.indices[idx].vertex_index;
                if (vertexOrder == -1)
                    break;
                
                auto vertexIdx = vertexOrder * vertexNumberPerFace;

                const float x = attrib.vertices[vertexIdx + 0];
                const float y = attrib.vertices[vertexIdx + 1];
                const float z = attrib.vertices[vertexIdx + 2];

                // Update min/max values 
                if (x < minX)
                    minX = x;
                if (x > maxX)
                    maxX = x;
                if (y < minY)
                    minY = y;
                if (y > maxY)
                    maxY = y;
                if (z < minZ)
                    minZ = z;
                if (z > maxZ)
                    maxZ = z;

                Model3D::VertexInfo vertex;
                vertex.xyz[0] = x;
                vertex.xyz[1] = y;
                vertex.xyz[2] = z;

                if (materialIdx != -1)
                {
                    const auto& faceMaterial = materials[materialIdx];
                    vertex.rgba[0] = faceMaterial.diffuse[0];
                    vertex.rgba[1] = faceMaterial.diffuse[1];
                    vertex.rgba[2] = faceMaterial.diffuse[2];
                    vertex.rgba[3] = faceMaterial.dissolve;
                    vertex.materialName = QString::fromStdString(faceMaterial.name);
                }
                else
                {
                    vertex.rgba[0] = attrib.colors[vertexIdx + 0];
                    vertex.rgba[1] = attrib.colors[vertexIdx + 1];
                    vertex.rgba[2] = attrib.colors[vertexIdx + 2];
                    vertex.rgba[3] = 1.0f;
                }

                faceVertices.push_back(vertex);
            }

            if (faceVertices.length() == vertexNumberPerFace)
                for (const auto& vertex : faceVertices)
                    vertices.push_back(vertex);
        }
    }

    if (minX == maxX || minY == maxY || minZ == maxZ)
    {
        LogPrintf(LogSeverityLevel::Error, "3D Model is flat in one of axis");
        return false;
    }

    if (translateToOrigin)
    {
        const auto translateX = -(minX + (maxX - minX) / 2.0f);
        const auto translateY = -minY;
        const auto translateZ = -(minZ + (maxZ - minZ) / 2.0f);

        for (size_t i = 0; i < vertices.size(); i++)
        {
            auto& vertex = vertices[i];
            vertex.xyz[0] += translateX;
            vertex.xyz[1] += translateY;
            vertex.xyz[2] += translateZ;
        }

        minX += translateX;
        maxX += translateX;
        minY += translateY;
        maxY += translateY;
        minZ += translateZ;
        maxZ += translateZ;
    }

    Model3D::BBox bbox(minX, maxX, minY, maxY, minZ, maxZ);
    outModel.reset(new Model3D(vertices, bbox));

    return true;
}