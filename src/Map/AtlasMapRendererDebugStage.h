#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_H_

#include "stdlib_common.h"
#include <tuple>

#include <glm/glm.hpp>

#include "QtExtensions.h"
#include <QList>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererStage.h"

namespace OsmAnd
{
    class AtlasMapRenderer;

    class AtlasMapRendererDebugStage : public AtlasMapRendererStage
    {
    private:
    protected:
        typedef std::pair<PointF, uint32_t> Point2D;
        QList<Point2D> _points2D;

        typedef std::tuple<AreaF, uint32_t, float> Rect2D;
        QList<Rect2D> _rects2D;
        
        typedef std::pair< QVector<glm::vec2>, uint32_t> Line2D;
        QList<Line2D> _lines2D;
        
        typedef std::pair< QVector<glm::vec3>, uint32_t> Line3D;
        QList<Line3D> _lines3D;
        
        typedef std::tuple< glm::vec3, glm::vec3, glm::vec3, glm::vec3, uint32_t> Quad3D;
        QList<Quad3D> _quads3D;
    public:
        AtlasMapRendererDebugStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRendererDebugStage();

        virtual void clear();
        virtual void addPoint2D(const PointF& point, const uint32_t argbColor);
        virtual void addRect2D(const AreaF& rect, const uint32_t argbColor, const float angle = 0.0f);
        virtual void addLine2D(const QVector<glm::vec2>& line, const uint32_t argbColor);
        virtual void addLine3D(const QVector<glm::vec3>& line, const uint32_t argbColor);
        virtual void addQuad3D(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const uint32_t argbColor);
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_OPENGL_H_)