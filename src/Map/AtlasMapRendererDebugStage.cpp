#include "AtlasMapRendererDebugStage.h"

OsmAnd::AtlasMapRendererDebugStage::AtlasMapRendererDebugStage(AtlasMapRenderer* const renderer_)
    : AtlasMapRendererStage(renderer_)
{
}

OsmAnd::AtlasMapRendererDebugStage::~AtlasMapRendererDebugStage()
{
}

void OsmAnd::AtlasMapRendererDebugStage::clear()
{
    _points2D.clear();
    _rects2D.clear();
    _lines2D.clear();
    _lines3D.clear();
    _quads3D.clear();
}

void OsmAnd::AtlasMapRendererDebugStage::addPoint2D(const PointF& point, const uint32_t argbColor)
{
    _points2D.push_back(qMove(Point2D(point, argbColor)));
}

void OsmAnd::AtlasMapRendererDebugStage::addRect2D(const AreaF& rect, const uint32_t argbColor, const float angle /*= 0.0f*/)
{
    _rects2D.push_back(qMove(Rect2D(rect, argbColor, angle)));
}

void OsmAnd::AtlasMapRendererDebugStage::addLine2D(const QVector<glm::vec2>& line, const uint32_t argbColor)
{
    assert(line.size() >= 2);
    _lines2D.push_back(qMove(Line2D(line, argbColor)));
}

void OsmAnd::AtlasMapRendererDebugStage::addLine3D(const QVector<glm::vec3>& line, const uint32_t argbColor)
{
    assert(line.size() >= 2);
    _lines3D.push_back(qMove(Line3D(line, argbColor)));
}

void OsmAnd::AtlasMapRendererDebugStage::addQuad3D(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const uint32_t argbColor)
{
    _quads3D.push_back(qMove(Quad3D(p0, p1, p2, p3, argbColor)));
}
