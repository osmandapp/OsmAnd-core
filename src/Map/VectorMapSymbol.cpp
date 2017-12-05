#include "VectorMapSymbol.h"

OsmAnd::VectorMapSymbol::VectorMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : MapSymbol(group_)
    , vertices(nullptr)
    , verticesCount(0)
    , indices(nullptr)
    , indicesCount(0)
    , primitiveType(PrimitiveType::Invalid)
    , scaleType(ScaleType::Raw)
    , scale(1.0f)
{
}

OsmAnd::VectorMapSymbol::~VectorMapSymbol()
{
    releaseVerticesAndIndices();
}

void OsmAnd::VectorMapSymbol::releaseVerticesAndIndices()
{
    if (vertices != nullptr)
    {
        delete[] vertices;
        vertices = nullptr;
    }
    verticesCount = 0;

    if (indices != nullptr)
    {
        delete[] indices;
        indices = nullptr;
    }
    indicesCount = 0;
}

void OsmAnd::VectorMapSymbol::generateCirclePrimitive(
    VectorMapSymbol& mapSymbol,
    const FColorARGB color /*= FColorARGB(1.0f, 1.0f, 1.0f, 1.0f)*/,
    const unsigned int pointsCount /*= 360*/,
    float radius /*= 1.0f*/)
{
    mapSymbol.releaseVerticesAndIndices();

    if (pointsCount == 0)
        return;

    mapSymbol.primitiveType = PrimitiveType::TriangleFan;

    // Circle has no reusable vertices, because it's rendered as triangle-fan,
    // so there's no indices
    mapSymbol.indices = nullptr;
    mapSymbol.indicesCount = 0;

    // Allocate space for pointsCount+2 vertices
    mapSymbol.verticesCount = pointsCount + 2;
    mapSymbol.vertices = new Vertex[mapSymbol.verticesCount];
    auto pVertex = mapSymbol.vertices;

    // First vertex is the center
    pVertex->positionXY[0] = 0.0f;
    pVertex->positionXY[1] = 0.0f;
    pVertex->color = color;
    pVertex += 1;

    // Generate each next vertex except the last one
    const auto step = (2.0*M_PI) / pointsCount;
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        const auto angle = step * pointIdx;
        pVertex->positionXY[0] = radius * qCos(angle);
        pVertex->positionXY[1] = radius * qSin(angle);
        pVertex->color = color;

        pVertex += 1;
    }

    // Close the fan
    pVertex->positionXY[0] = mapSymbol.vertices[1].positionXY[0];
    pVertex->positionXY[1] = mapSymbol.vertices[1].positionXY[1];
    pVertex->color = color;
}

void OsmAnd::VectorMapSymbol::generateRingLinePrimitive(
    VectorMapSymbol& mapSymbol,
    const FColorARGB color /*= FColorARGB(1.0f, 1.0f, 1.0f, 1.0f)*/,
    const unsigned int pointsCount /*= 360*/,
    float radius /*= 1.0f*/)
{
    mapSymbol.releaseVerticesAndIndices();

    if (pointsCount == 0)
        return;

    mapSymbol.primitiveType = PrimitiveType::LineLoop;

    // Ring as line-loop has no reusable vertices, because it's rendered as triangle-fan,
    // so there's no indices
    mapSymbol.indices = nullptr;
    mapSymbol.indicesCount = 0;

    // Allocate space for pointsCount vertices
    mapSymbol.verticesCount = pointsCount;
    mapSymbol.vertices = new Vertex[mapSymbol.verticesCount];
    auto pVertex = mapSymbol.vertices;

    // Generate each vertex
    const auto step = (2.0*M_PI) / pointsCount;
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        const auto angle = step * pointIdx;
        pVertex->positionXY[0] = radius * qCos(angle);
        pVertex->positionXY[1] = radius * qSin(angle);
        pVertex->color = color;

        pVertex += 1;
    }
}

void OsmAnd::VectorMapSymbol::generateLinePrimitive(
    VectorMapSymbol& mapSymbol,
    const float lineWidth /*= 3.0f*/,
    const FColorARGB color /*= FColorARGB(1.0f, 1.0f, 1.0f, 1.0f)*/)
{

}
