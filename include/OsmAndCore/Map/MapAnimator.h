#ifndef _OSMAND_CORE_MAP_ANIMATOR_H_
#define _OSMAND_CORE_MAP_ANIMATOR_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/IAnimation.h>

namespace OsmAnd
{
    class IMapRenderer;

    class MapAnimator_P;
    class OSMAND_CORE_API MapAnimator
    {
        Q_DISABLE_COPY_AND_MOVE(MapAnimator);
    public:
        typedef Animator::TimingFunction TimingFunction;
        typedef Animator::AnimatedValue AnimatedValue;
        typedef Animator::Key Key;

    private:
        PrivateImplementation<MapAnimator_P> _p;
    protected:
    public:
        MapAnimator(
            const bool suspendSymbolsDuringAnimation = true);
        virtual ~MapAnimator();

        const bool suspendSymbolsDuringAnimation;

        void setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer);
        const std::shared_ptr<IMapRenderer>& mapRenderer;

        bool isPaused() const;
        void pause();
        void resume();

        bool cancelAnimation(const std::shared_ptr<const IAnimation>& animation);

        QList< std::shared_ptr<IAnimation> > getAnimations(const Key key);
        QList< std::shared_ptr<const IAnimation> > getAnimations(const Key key) const;
        bool pauseAnimations(const Key key);
        bool resumeAnimations(const Key key);
        bool cancelAnimations(const Key key);

        bool cancelCurrentAnimation(const Key key, const AnimatedValue animatedValue);
        std::shared_ptr<IAnimation> getCurrentAnimation(const Key key, const AnimatedValue animatedValue);
        std::shared_ptr<const IAnimation> getCurrentAnimation(const Key key, const AnimatedValue animatedValue) const;

        QList< std::shared_ptr<IAnimation> > getAllAnimations();
        QList< std::shared_ptr<const IAnimation> > getAllAnimations() const;
        void cancelAllAnimations();

        bool update(const float timePassed);

        void animateZoomBy(
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateZoomTo(
            const float value,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateZoomWith(
            const float velocity,
            const float deceleration,
            const Key key = nullptr);
        void animateZoomToAndPan(
            const float value,
            const PointI& panValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);

        void animateTargetBy(
            const PointI64& deltaValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateTargetTo(
            const PointI& value,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateTargetWith(
            const PointD& velocity,
            const PointD& deceleration,
            const Key key = nullptr);

        void parabolicAnimateTargetBy(
            const PointI64& deltaValue,
            const float duration,
            const TimingFunction targetTimingFunction,
            const TimingFunction zoomTimingFunction,
            const Key key = nullptr);
        void parabolicAnimateTargetTo(
            const PointI& value,
            const float duration,
            const TimingFunction targetTimingFunction,
            const TimingFunction zoomTimingFunction,
            const Key key = nullptr);
        void parabolicAnimateTargetWith(
            const PointD& velocity,
            const PointD& deceleration,
            const Key key = nullptr);

        void animateFlatTargetWith(
            const PointD& velocity,
            const PointD& deceleration,
            const Key key = nullptr);

        void animateSecondaryTargetBy(
            const PointI64& deltaValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateSecondaryTargetTo(
            const PointI& value,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateSecondaryTargetWith(
            const PointD& velocity,
            const PointD& deceleration,
            const Key key = nullptr);

        void animateAzimuthBy(
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateAzimuthTo(
            const float value,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateAzimuthWith(
            const float velocity,
            const float deceleration,
            const Key key = nullptr);

        void animateElevationAngleBy(
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateElevationAngleTo(
            const float value,
            const float duration,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateElevationAngleWith(
            const float velocity,
            const float deceleration,
            const Key key = nullptr);

        void animateMoveBy(
            const PointI64& deltaValue,
            const float duration,
            const bool zeroizeAzimuth,
            const bool invZeroizeElevationAngle,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateMoveTo(
            const PointI& deltaValue,
            const float duration,
            const bool zeroizeAzimuth,
            const bool invZeroizeElevationAngle,
            const TimingFunction timingFunction,
            const Key key = nullptr);
        void animateMoveWith(
            const PointD& velocity,
            const PointD& deceleration,
            const bool zeroizeAzimuth,
            const bool invZeroizeElevationAngle,
            const Key key = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_ANIMATOR_H_)
