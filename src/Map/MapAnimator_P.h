#ifndef _OSMAND_CORE_MAP_ANIMATOR_P_H_
#define _OSMAND_CORE_MAP_ANIMATOR_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QtMath>
#include <QHash>
#include <QMap>
#include <QList>
#include <QReadWriteLock>
#include <QMutex>
#include <QVariant>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "Animation.h"
#include "MapAnimator.h"
#include "MapCommonTypes.h"

namespace OsmAnd
{
    class IMapRenderer;

    class MapAnimator;
    class MapAnimator_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapAnimator_P);
    public:
        typedef Animator::TimingFunction TimingFunction;
        typedef Animator::AnimatedValue AnimatedValue;
        typedef Animator::Key Key;

    private:
    protected:
        MapAnimator_P(MapAnimator* const owner);

        std::shared_ptr<IMapRenderer> _renderer;
        bool _rendererSymbolsUpdateSuspended;

        typedef QList< std::shared_ptr<GenericAnimation> > AnimationsCollection;

        mutable QMutex _updateLock;
        volatile bool _isPaused;

        mutable QReadWriteLock _animationsCollectionLock;
        QHash<Key, AnimationsCollection> _animationsByKey;
        bool _inconsistentMapTarget;

        void constructZoomAnimationByDelta(
            AnimationsCollection& outAnimation,
            const Key key,
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction);
        void constructZoomAnimationToValue(
            AnimationsCollection& outAnimation,
            const Key key,
            const float value,
            const float duration,
            const TimingFunction timingFunction);
        void constructZoomAnimationToValueAndPan(
            AnimationsCollection& outAnimation,
            const Key key,
            const float value,
            const PointI& panValue,
            const float duration,
            const TimingFunction timingFunction);

        void constructFlatTargetAnimationByDelta(
            AnimationsCollection& outAnimation,
            const Key key,
            const PointI64& deltaValue,
            const float duration,
            const TimingFunction timingFunction);

        void constructTargetAnimationByDelta(
            AnimationsCollection& outAnimation,
            const Key key,
            const PointI64& deltaValue,
            const float duration,
            const TimingFunction timingFunction);
        void constructTargetAnimationToValue(
            AnimationsCollection& outAnimation,
            const Key key,
            const PointI& value,
            const float duration,
            const TimingFunction timingFunction);

        void constructParabolicTargetAnimationByDelta(
            AnimationsCollection& outAnimation,
            const Key key,
            const PointI64& deltaValue,
            const float duration,
            const TimingFunction targetTimingFunction,
            const TimingFunction zoomTimingFunction);
        void constructParabolicTargetAnimationByDelta_Zoom(
            AnimationsCollection& outAnimation,
            const Key key,
            const PointI64& targetDeltaValue,
            const float duration,
            const TimingFunction zoomTimingFunction);
        void constructParabolicTargetAnimationToValue(
            AnimationsCollection& outAnimation,
            const Key key,
            const PointI& value,
            const float duration,
            const TimingFunction targetTimingFunction,
            const TimingFunction zoomTimingFunction);
        void constructParabolicTargetAnimationToValue_Zoom(
            AnimationsCollection& outAnimation,
            const Key key,
            const PointI& targetValue,
            const float duration,
            const TimingFunction zoomTimingFunction);

        void constructAzimuthAnimationByDelta(
            AnimationsCollection& outAnimation,
            const Key key,
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction);
        void constructAzimuthAnimationToValue(
            AnimationsCollection& outAnimation,
            const Key key,
            const float value,
            const float duration,
            const TimingFunction timingFunction);

        void constructElevationAngleAnimationByDelta(
            AnimationsCollection& outAnimation,
            const Key key,
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction);
        void constructElevationAngleAnimationToValue(
            AnimationsCollection& outAnimation,
            const Key key,
            const float value,
            const float duration,
            const TimingFunction timingFunction);

        void constructZeroizeAzimuthAnimation(
            AnimationsCollection& outAnimation,
            const Key key,
            const float duration,
            const TimingFunction timingFunction);
        void constructInvZeroizeElevationAngleAnimation(
            AnimationsCollection& outAnimation,
            const Key key,
            const float duration,
            const TimingFunction timingFunction);

        const Animation<float>::GetInitialValueMethod _zoomGetter;
        float zoomGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);
        const Animation<float>::ApplierMethod _zoomSetter;
        void zoomSetter(const Key key, const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);

        const Animation<float>::GetInitialValueMethod _azimuthGetter;
        float azimuthGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);
        const Animation<float>::ApplierMethod _azimuthSetter;
        void azimuthSetter(const Key key, const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);

        const Animation<float>::GetInitialValueMethod _elevationAngleGetter;
        float elevationAngleGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);
        const Animation<float>::ApplierMethod _elevationAngleSetter;
        void elevationAngleSetter(const Key key, const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);

        const Animation<PointI64>::GetInitialValueMethod _targetGetter;
        PointI64 targetGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);
        const Animation<PointI64>::ApplierMethod _targetSetter;
        void targetSetter(const Key key, const PointI64 newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);

        const Animation<PointI64>::GetInitialValueMethod _flatTargetGetter;
        PointI64 flatTargetGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);
        const Animation<PointI64>::ApplierMethod _flatTargetSetter;
        void flatTargetSetter(const Key key, const PointI64 newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);

        static std::shared_ptr<GenericAnimation> findCurrentAnimation(const AnimatedValue animatedValue, const AnimationsCollection& collection);
    public:
        ~MapAnimator_P();

        ImplementationInterface<MapAnimator> owner;

        void setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer);

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
        void invalidateMapTarget();

        bool update(const float timePassed);

        void animateZoomBy(
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key);
        void animateZoomTo(
            const float value,
            const float duration,
            const TimingFunction timingFunction,
            const Key key);
        void animateZoomWith(
            const float velocity,
            const float deceleration,
            const Key key);
        void animateZoomToAndPan(
            const float value,
            const PointI& panValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key);

        void animateTargetBy(
            const PointI64& deltaValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key);
        void animateTargetTo(
            const PointI& value,
            const float duration,
            const TimingFunction timingFunction,
            const Key key);
        void animateTargetWith(
            const PointD& velocity,
            const PointD& deceleration,
            const Key key);

        void parabolicAnimateTargetBy(
            const PointI64& deltaValue,
            const float duration,
            const TimingFunction targetTimingFunction,
            const TimingFunction zoomTimingFunction,
            const Key key);
        void parabolicAnimateTargetTo(
            const PointI& value,
            const float duration,
            const TimingFunction targetTimingFunction,
            const TimingFunction zoomTimingFunction,
            const Key key);
        void parabolicAnimateTargetWith(
            const PointD& velocity,
            const PointD& deceleration,
            const Key key);

        void animateFlatTargetWith(
            const PointD& velocity,
            const PointD& deceleration,
            const Key key);

        void animateAzimuthBy(
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key);
        void animateAzimuthTo(
            const float value,
            const float duration,
            const TimingFunction timingFunction,
            const Key key);
        void animateAzimuthWith(
            const float velocity,
            const float deceleration,
            const Key key);

        void animateElevationAngleBy(
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction,
            const Key key);
        void animateElevationAngleTo(
            const float value,
            const float duration,
            const TimingFunction timingFunction,
            const Key key);
        void animateElevationAngleWith(
            const float velocity,
            const float deceleration,
            const Key key);

        void animateMoveBy(
            const PointI64& deltaValue,
            const float duration,
            const bool zeroizeAzimuth,
            const bool invZeroizeElevationAngle,
            const TimingFunction timingFunction,
            const Key key);
        void animateMoveTo(
            const PointI& value,
            const float duration,
            const bool zeroizeAzimuth,
            const bool invZeroizeElevationAngle,
            const TimingFunction timingFunction,
            const Key key);
        void animateMoveWith(
            const PointD& velocity,
            const PointD& deceleration,
            const bool zeroizeAzimuth,
            const bool invZeroizeElevationAngle,
            const Key key);

    friend class OsmAnd::MapAnimator;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_ANIMATOR_P_H_)
