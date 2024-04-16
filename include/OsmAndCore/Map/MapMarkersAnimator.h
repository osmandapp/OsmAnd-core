#ifndef _OSMAND_CORE_MAP_MARKERS_ANIMATOR_H_
#define _OSMAND_CORE_MAP_MARKERS_ANIMATOR_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/IAnimation.h>
#include <OsmAndCore/Map/MapMarker.h>

namespace OsmAnd
{
    class IMapRenderer;

    class MapMarkersAnimator_P;
    class OSMAND_CORE_API MapMarkersAnimator
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarkersAnimator);
    public:
        typedef Animator::TimingFunction TimingFunction;
        typedef Animator::AnimatedValue AnimatedValue;
        typedef Animator::Key Key;
        typedef MapMarker::OnSurfaceIconKey OnSurfaceIconKey;

    private:
        PrivateImplementation<MapMarkersAnimator_P> _p;
    protected:
    public:
        MapMarkersAnimator();
        virtual ~MapMarkersAnimator();

        void setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer);
        const std::shared_ptr<IMapRenderer>& mapRenderer;

        bool isPaused() const;
        void pause();
        void resume();

        bool cancelAnimation(const std::shared_ptr<const IAnimation>& animation);

        QList< std::shared_ptr<IAnimation> > getAnimations(const std::shared_ptr<MapMarker> mapMarker);
        QList< std::shared_ptr<const IAnimation> > getAnimations(const std::shared_ptr<MapMarker> mapMarker) const;
        bool pauseAnimations(const std::shared_ptr<MapMarker> mapMarker);
        bool resumeAnimations(const std::shared_ptr<MapMarker> mapMarker);
        bool cancelAnimations(const std::shared_ptr<MapMarker> mapMarker);

        bool cancelCurrentAnimation(const std::shared_ptr<MapMarker> mapMarker, const AnimatedValue animatedValue);
        std::shared_ptr<IAnimation> getCurrentAnimation(const std::shared_ptr<MapMarker> mapMarker, const AnimatedValue animatedValue);
        std::shared_ptr<const IAnimation> getCurrentAnimation(const std::shared_ptr<MapMarker> mapMarker, const AnimatedValue animatedValue) const;

        QList< std::shared_ptr<IAnimation> > getAllAnimations();
        QList< std::shared_ptr<const IAnimation> > getAllAnimations() const;
        void cancelAllAnimations();

        bool update(const float timePassed);

        void animatePositionBy(
            const std::shared_ptr<MapMarker> mapMarker,
            const PointI64& deltaValue,
            const float duration,
            const TimingFunction timingFunction);
        void animatePositionTo(
            const std::shared_ptr<MapMarker> mapMarker,
            const PointI& value,
            const float duration,
            const TimingFunction timingFunction);
        void animatePositionWith(
            const std::shared_ptr<MapMarker> mapMarker,
            const PointD& velocity,
            const PointD& deceleration);

        void animateDirectionBy(
            const std::shared_ptr<MapMarker> mapMarker,
            const OnSurfaceIconKey iconKey,
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction);
        void animateDirectionTo(
            const std::shared_ptr<MapMarker> mapMarker,
            const OnSurfaceIconKey iconKey,
            const float value,
            const float duration,
            const TimingFunction timingFunction);
        void animateDirectionWith(
            const std::shared_ptr<MapMarker> mapMarker,
            const OnSurfaceIconKey iconKey,
            const float velocity,
            const float deceleration);

        void animateModel3DDirectionBy(
            const std::shared_ptr<MapMarker> mapMarker,
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction);
        void animateModel3DDirectionTo(
            const std::shared_ptr<MapMarker> mapMarker,
            const float value,
            const float duration,
            const TimingFunction timingFunction);
        void animateModel3DDirectionWith(
            const std::shared_ptr<MapMarker> mapMarker,
            const float velocity,
            const float deceleration);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKERS_ANIMATOR_H_)
