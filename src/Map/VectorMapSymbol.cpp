#include "VectorMapSymbol.h"

OsmAnd::VectorMapSymbol::VectorMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : MapSymbol(group_)
    , _verticesAndIndexes(nullptr)
    , primitiveType(PrimitiveType::Invalid)
    , scaleType(ScaleType::Raw)
    , scale(1.0f)
{
}

OsmAnd::VectorMapSymbol::~VectorMapSymbol()
{
    releaseVerticesAndIndices();
}

OsmAnd::VectorMapSymbol::VerticesAndIndexes::VerticesAndIndexes()
    : vertices(nullptr)
    , verticesCount(0)
    , indices(nullptr)
    , indicesCount(0)
{
}

OsmAnd::VectorMapSymbol::VerticesAndIndexes::~VerticesAndIndexes()
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

const std::shared_ptr<OsmAnd::VectorMapSymbol::VerticesAndIndexes> OsmAnd::VectorMapSymbol::getVerticesAndIndexes() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _verticesAndIndexes;
}

void OsmAnd::VectorMapSymbol::setVerticesAndIndexes(const std::shared_ptr<VerticesAndIndexes>& verticesAndIndexes)
{
    QWriteLocker scopedLocker(&_lock);
    
    _verticesAndIndexes = verticesAndIndexes;
}

void OsmAnd::VectorMapSymbol::releaseVerticesAndIndices()
{
    QWriteLocker scopedLocker(&_lock);

    _verticesAndIndexes.reset();
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

    const auto verticesAndIndexes = std::make_shared<VerticesAndIndexes>();
    // Circle has no reusable vertices, because it's rendered as triangle-fan,
    // so there's no indices
    verticesAndIndexes->indices = nullptr;
    verticesAndIndexes->indicesCount = 0;

    // Allocate space for pointsCount+2 vertices
    verticesAndIndexes->verticesCount = pointsCount + 2;
    verticesAndIndexes->vertices = new Vertex[verticesAndIndexes->verticesCount];
    auto pVertex = verticesAndIndexes->vertices;

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
    pVertex->positionXY[0] = verticesAndIndexes->vertices[1].positionXY[0];
    pVertex->positionXY[1] = verticesAndIndexes->vertices[1].positionXY[1];
    pVertex->color = color;
    
    mapSymbol.setVerticesAndIndexes(verticesAndIndexes);
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

    const auto verticesAndIndexes = std::make_shared<VerticesAndIndexes>();
    // Ring as line-loop has no reusable vertices, because it's rendered as triangle-fan,
    // so there's no indices
    verticesAndIndexes->indices = nullptr;
    verticesAndIndexes->indicesCount = 0;

    // Allocate space for pointsCount vertices
    verticesAndIndexes->verticesCount = pointsCount;
    verticesAndIndexes->vertices = new Vertex[verticesAndIndexes->verticesCount];
    auto pVertex = verticesAndIndexes->vertices;

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
    
    mapSymbol.setVerticesAndIndexes(verticesAndIndexes);
}

