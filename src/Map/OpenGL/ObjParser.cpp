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

bool OsmAnd::ObjParser::parse(std::shared_ptr<Model3D>& outModel) const
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
        LogPrintf(LogSeverityLevel::Warning, qPrintable(QString::fromStdString(warn)));

    if (error.length() > 0)
        LogPrintf(LogSeverityLevel::Error, qPrintable(QString::fromStdString(error)));

    if (!ok)
        return false;

    if (attrib.vertices.size() == 0)
    {
        QFileInfo objFileInfo(_objFilename);
        LogPrintf(LogSeverityLevel::Error, "No vertices data provided in %s", qPrintable(objFileInfo.fileName()));
        return false;
    }

    if (materials.size() == 0 && attrib.colors.size() != attrib.vertices.size())
    {
        QFileInfo objFileInfo(_objFilename);
        LogPrintf(LogSeverityLevel::Error, "No color data provided in %s", qPrintable(objFileInfo.fileName()));
        return false;
    }

    QVector<Model3D::VertexInfo> vertices;
    Model3D::BBox bbox = {
        .minX = attrib.vertices[0],
        .maxX = bbox.minX,
        .minY = attrib.vertices[1],
        .maxY = bbox.minY,
        .minZ = attrib.vertices[2],
        .maxZ = bbox.minZ
    };
    for (auto shape : shapes)
    {
        auto mesh = shape.mesh;
        for (auto faceIdx = 0; faceIdx < mesh.num_face_vertices.size(); faceIdx++)
        {
            auto vertexNumberPerFace = mesh.num_face_vertices[faceIdx];
            assert(vertexNumberPerFace == 3); // Should be triangulated by tinyobj

            auto materialIdx = mesh.material_ids[faceIdx];
            tinyobj::material_t* faceMaterial = materialIdx == -1 ? nullptr : &materials[materialIdx]; 

            auto startIdx = faceIdx * vertexNumberPerFace;
            auto endIdx = startIdx + vertexNumberPerFace;

            QVector<Model3D::VertexInfo> faceVertices;

            for (auto idx = startIdx; idx < endIdx; idx++)
            {
                auto vertexOrder = mesh.indices[idx].vertex_index;
                if (vertexOrder == -1)
                    break;
                
                auto vertexIdx = vertexOrder * vertexNumberPerFace;
                Model3D::VertexInfo vertex = Model3D::VertexInfo {}; // or no?

                float x = attrib.vertices[vertexIdx + 0];
                float y = attrib.vertices[vertexIdx + 1];
                float z = attrib.vertices[vertexIdx + 2];

                vertex.xyz[0] = x;
                vertex.xyz[1] = y;
                vertex.xyz[2] = z;

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

                if (faceMaterial)
                {
                    vertex.rgba[0] = faceMaterial->diffuse[0];
                    vertex.rgba[1] = faceMaterial->diffuse[1];
                    vertex.rgba[2] = faceMaterial->diffuse[2];
                    vertex.rgba[3] = faceMaterial->dissolve;
                    vertex.materialName = QString::fromStdString(faceMaterial->name);
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
                for (auto vertex : faceVertices)
                    vertices.push_back(vertex);
        }
    }

    outModel.reset(new Model3D(vertices, bbox));

    return true;
}