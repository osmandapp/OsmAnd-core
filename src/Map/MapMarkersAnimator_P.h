#ifndef _OSMAND_CORE_MAP_MARKERS_ANIMATOR_P_H_
#define _OSMAND_CORE_MAP_MARKERS_ANIMATOR_P_H_

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
#include "MapMarkersAnimator.h"
#include "MapCommonTypes.h"

namespace OsmAnd
{
    class IMapRenderer;

    class MapMarkersAnimator;
    class MapMarkersAnimator_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarkersAnimator_P);
    public:
        typedef Animator::TimingFunction TimingFunction;
        typedef Animator::AnimatedValue AnimatedValue;
        typedef Animator::Key Key;
        typedef MapMarker::OnSurfaceIconKey OnSurfaceIconKey;

    private:
    protected:
        MapMarkersAnimator_P(MapMarkersAnimator* const owner);

        std::shared_ptr<IMapRenderer> _renderer;

        typedef QList< std::shared_ptr<GenericAnimation> > AnimationsCollection;

        mutable QMutex _updateLock;
        volatile bool _isPaused;

        mutable QReadWriteLock _animationsCollectionLock;
        QHash<Key, AnimationsCollection> _animationsByKey;
        mutable QReadWriteLock _animationMarkersLock;
        QHash<Key, std::weak_ptr<MapMarker>> _animationMarkers;

        void constructPositionAnimationByDelta(
            AnimationsCollection& outAnimation,
            const std::shared_ptr<MapMarker> mapMarker,
            const PointI64& deltaValue,
            const float duration,
            const TimingFunction timingFunction);
        void constructPositionAnimationToValue(
            AnimationsCollection& outAnimation,
            const std::shared_ptr<MapMarker> mapMarker,
            const PointI& value,
            const float duration,
            const TimingFunction timingFunction);

        void constructDirectionAnimationByDelta(
            AnimationsCollection& outAnimation,
            const std::shared_ptr<MapMarker> mapMarker,
            const OnSurfaceIconKey* const pIconKey,
            const float deltaValue,
            const float duration,
            const TimingFunction timingFunction);
        void constructDirectionAnimationToValue(
            AnimationsCollection& outAnimation,
            const std::shared_ptr<MapMarker> mapMarker,
            const OnSurfaceIconKey* const pIconKey,
            const float value,
            const float duration,
            const TimingFunction timingFunction);

        const Animation<float>::GetInitialValueMethod _directionGetter;
        float directionGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);
        const Animation<float>::ApplierMethod _directionSetter;
        void directionSetter(const Key key, const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);

        const Animation<PointI64>::GetInitialValueMethod _positionGetter;
        PointI64 positionGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);
        const Animation<PointI64>::ApplierMethod _positionSetter;
        void positionSetter(const Key key, const PointI64 newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext);

        static std::shared_ptr<GenericAnimation> findCurrentAnimation(const AnimatedValue animatedValue, const AnimationsCollection& collection);
    public:
        ~MapMarkersAnimator_P();

        ImplementationInterface<MapMarkersAnimator> owner;

        void setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer);

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

    friend class OsmAnd::MapMarkersAnimator;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKERS_ANIMATOR_P_H_)
