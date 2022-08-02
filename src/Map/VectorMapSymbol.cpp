#include "VectorMapSymbol.h"

OsmAnd::VectorMapSymbol::VectorMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : MapSymbol(group_)
    , _verticesAndIndices(nullptr)
    , primitiveType(PrimitiveType::Invalid)
    , scaleType(ScaleType::Raw)
    , scale(1.0f)
{
}

OsmAnd::VectorMapSymbol::~VectorMapSymbol()
{
    releaseVerticesAndIndices();
}

OsmAnd::VectorMapSymbol::VerticesAndIndices::VerticesAndIndices()
: position31(nullptr)
, vertices(nullptr)
, verticesCount(0)
, indices(nullptr)
, indicesCount(0)
, partSizes(nullptr)
{
}

OsmAnd::VectorMapSymbol::VerticesAndIndices::~VerticesAndIndices()
{
    if (position31 != nullptr)
    {
        delete position31;
        position31 = nullptr;
    }
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

    if (partSizes != nullptr)
        partSizes = nullptr;
}

const std::shared_ptr<OsmAnd::VectorMapSymbol::VerticesAndIndices> OsmAnd::VectorMapSymbol::getVerticesAndIndices() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _verticesAndIndices;
}

void OsmAnd::VectorMapSymbol::setVerticesAndIndices(const std::shared_ptr<VerticesAndIndices>& verticesAndIndices)
{
    QWriteLocker scopedLocker(&_lock);
    
    _verticesAndIndices = verticesAndIndices;
}

void OsmAnd::VectorMapSymbol::releaseVerticesAndIndices()
{
    QWriteLocker scopedLocker(&_lock);

    _verticesAndIndices.reset();
}

void OsmAnd::VectorMapSymbol::generateCirclePrimitive(
    VectorMapSymbol& mapSymbol,
    const FColorARGB color /*= FColorARGB(1.0f, 1.0f, 1.0f, 1.0f)*/,
    const unsigned int pointsCount /*= 360*/,
    float radius /*= 1.0f*/)
{
    if (pointsCount == 0)
    {
        mapSymbol.releaseVerticesAndIndices();
        return;
    }

    mapSymbol.primitiveType = PrimitiveType::TriangleFan;

    const auto verticesAndIndices = std::make_shared<VerticesAndIndices>();
    // Circle has no reusable vertices, because it's rendered as triangle-fan,
    // so there's no indices
    verticesAndIndices->indices = nullptr;
    verticesAndIndices->indicesCount = 0;

    // Allocate space for pointsCount+2 vertices
    verticesAndIndices->verticesCount = pointsCount + 2;
    verticesAndIndices->vertices = new Vertex[verticesAndIndices->verticesCount];
    auto pVertex = verticesAndIndices->vertices;

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
    pVertex->positionXY[0] = verticesAndIndices->vertices[1].positionXY[0];
    pVertex->positionXY[1] = verticesAndIndices->vertices[1].positionXY[1];
    pVertex->color = color;
    
    mapSymbol.setVerticesAndIndices(verticesAndIndices);
}

void OsmAnd::VectorMapSymbol::generateRingLinePrimitive(
    VectorMapSymbol& mapSymbol,
    const FColorARGB color /*= FColorARGB(1.0f, 1.0f, 1.0f, 1.0f)*/,
    const unsigned int pointsCount /*= 360*/,
    float radius /*= 1.0f*/)
{
    if (pointsCount == 0)
    {
        mapSymbol.releaseVerticesAndIndices();
        return;
    }

    mapSymbol.primitiveType = PrimitiveType::LineLoop;

    const auto verticesAndIndices = std::make_shared<VerticesAndIndices>();
    // Ring as line-loop has no reusable vertices, because it's rendered as triangle-fan,
    // so there's no indices
    verticesAndIndices->indices = nullptr;
    verticesAndIndices->indicesCount = 0;

    // Allocate space for pointsCount vertices
    verticesAndIndices->verticesCount = pointsCount;
    verticesAndIndices->vertices = new Vertex[verticesAndIndices->verticesCount];
    auto pVertex = verticesAndIndices->vertices;

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
    
    mapSymbol.setVerticesAndIndices(verticesAndIndices);
}

