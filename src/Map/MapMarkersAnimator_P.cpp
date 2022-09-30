#include "MapMarkersAnimator_P.h"
#include "MapMarkersAnimator.h"

#include "QtCommon.h"

#include "IMapRenderer.h"
#include "Utilities.h"

OsmAnd::MapMarkersAnimator_P::MapMarkersAnimator_P( MapMarkersAnimator* const owner_ )
    : _isPaused(false)
    , _directionGetter(std::bind(&MapMarkersAnimator_P::directionGetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _directionSetter(std::bind(&MapMarkersAnimator_P::directionSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
    , _positionGetter(std::bind(&MapMarkersAnimator_P::positionGetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _positionSetter(std::bind(&MapMarkersAnimator_P::positionSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
    , owner(owner_)
{
}

OsmAnd::MapMarkersAnimator_P::~MapMarkersAnimator_P()
{
}

void OsmAnd::MapMarkersAnimator_P::setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer)
{
    QMutexLocker scopedLocker(&_updateLock);

    _isPaused = false;
    _animationsByKey.clear();
    _animationMarkers.clear();
    _renderer = mapRenderer;
}

bool OsmAnd::MapMarkersAnimator_P::isPaused() const
{
    return _isPaused;
}

void OsmAnd::MapMarkersAnimator_P::pause()
{
    _isPaused = true;
}

void OsmAnd::MapMarkersAnimator_P::resume()
{
    _isPaused = false;
}

bool OsmAnd::MapMarkersAnimator_P::cancelAnimation(const std::shared_ptr<const IAnimation>& animation)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    const auto itAnimations = _animationsByKey.find(animation->getKey());
    if (itAnimations == _animationsByKey.end())
        return false;
    auto& animations = *itAnimations;

    bool wasRemoved = false;
    {
        auto itOtherAnimation = mutableIteratorOf(animations);
        while (itOtherAnimation.hasNext())
        {
            const auto& otherAnimation = itOtherAnimation.next();

            if (otherAnimation != animation)
                continue;

            itOtherAnimation.remove();
            wasRemoved = true;
            break;
        }
    }

    if (animations.isEmpty())
    {
        _animationsByKey.erase(itAnimations);

        QWriteLocker scopedMarkersLocker(&_animationMarkersLock);

        const auto itMarker = _animationMarkers.find(animation->getKey());
        if (itMarker != _animationMarkers.end())
            _animationMarkers.erase(itMarker);
    }

    return wasRemoved;
}

QList< std::shared_ptr<OsmAnd::IAnimation> > OsmAnd::MapMarkersAnimator_P::getAnimations(const std::shared_ptr<MapMarker> mapMarker)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(mapMarker.get());
    if (citAnimations == _animationsByKey.cend())
        return QList< std::shared_ptr<IAnimation> >();

    return copyAs< QList< std::shared_ptr<IAnimation> > >(*citAnimations);
}

QList< std::shared_ptr<const OsmAnd::IAnimation> > OsmAnd::MapMarkersAnimator_P::getAnimations(const std::shared_ptr<MapMarker> mapMarker) const
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(mapMarker.get());
    if (citAnimations == _animationsByKey.cend())
        return QList< std::shared_ptr<const IAnimation> >();

    return copyAs< QList< std::shared_ptr<const IAnimation> > >(*citAnimations);
}

bool OsmAnd::MapMarkersAnimator_P::pauseAnimations(const std::shared_ptr<MapMarker> mapMarker)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(mapMarker.get());
    if (citAnimations == _animationsByKey.cend())
        return false;
    const auto& animations = *citAnimations;

    for (const auto& animation : constOf(animations))
        animation->pause();

    return true;
}

bool OsmAnd::MapMarkersAnimator_P::resumeAnimations(const std::shared_ptr<MapMarker> mapMarker)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(mapMarker.get());
    if (citAnimations == _animationsByKey.cend())
        return false;
    const auto& animations = *citAnimations;

    for (const auto& animation : constOf(animations))
        animation->resume();

    return true;
}

bool OsmAnd::MapMarkersAnimator_P::cancelAnimations(const std::shared_ptr<MapMarker> mapMarker)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    const auto itAnimations = _animationsByKey.find(mapMarker.get());
    if (itAnimations == _animationsByKey.end())
        return false;

    _animationsByKey.erase(itAnimations);

    QWriteLocker scopedMarkersLocker(&_animationMarkersLock);

    const auto itMarker = _animationMarkers.find(mapMarker.get());
    if (itMarker != _animationMarkers.end())
        _animationMarkers.erase(itMarker);

    return true;
}

bool OsmAnd::MapMarkersAnimator_P::cancelCurrentAnimation(const std::shared_ptr<MapMarker> mapMarker, const AnimatedValue animatedValue)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    const auto itAnimations = _animationsByKey.find(mapMarker.get());
    if (itAnimations == _animationsByKey.end())
        return false;
    auto& animations = *itAnimations;

    bool wasRemoved = false;
    {
        auto itAnimation = mutableIteratorOf(animations);
        while (itAnimation.hasNext())
        {
            const auto& animation = itAnimation.next();

            if (animation->getAnimatedValue() != animatedValue || !animation->isPlaying())
                continue;

            itAnimation.remove();
            wasRemoved = true;
            break;
        }
    }

    if (animations.isEmpty())
    {
        _animationsByKey.erase(itAnimations);

        QWriteLocker scopedMarkersLocker(&_animationMarkersLock);

        const auto itMarker = _animationMarkers.find(mapMarker.get());
        if (itMarker != _animationMarkers.end())
            _animationMarkers.erase(itMarker);
    }

    return wasRemoved;
}

std::shared_ptr<OsmAnd::IAnimation> OsmAnd::MapMarkersAnimator_P::getCurrentAnimation(const std::shared_ptr<MapMarker> mapMarker, const AnimatedValue animatedValue)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(mapMarker.get());
    if (citAnimations == _animationsByKey.cend())
        return nullptr;

    return findCurrentAnimation(animatedValue, *citAnimations);
}

std::shared_ptr<const OsmAnd::IAnimation> OsmAnd::MapMarkersAnimator_P::getCurrentAnimation(const std::shared_ptr<MapMarker> mapMarker, const AnimatedValue animatedValue) const
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(mapMarker.get());
    if (citAnimations == _animationsByKey.cend())
        return nullptr;

    return findCurrentAnimation(animatedValue, *citAnimations);
}

QList< std::shared_ptr<OsmAnd::IAnimation> > OsmAnd::MapMarkersAnimator_P::getAllAnimations()
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    QList< std::shared_ptr<IAnimation> > result;
    for (const auto& animations : constOf(_animationsByKey))
        for (const auto& animation : constOf(animations))
            result.push_back(animation);

    return result;
}

QList< std::shared_ptr<const OsmAnd::IAnimation> > OsmAnd::MapMarkersAnimator_P::getAllAnimations() const
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    QList< std::shared_ptr<const IAnimation> > result;
    for (const auto& animations : constOf(_animationsByKey))
        for (const auto& animation : constOf(animations))
            result.push_back(animation);

    return result;
}

void OsmAnd::MapMarkersAnimator_P::cancelAllAnimations()
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    _animationsByKey.clear();

    QWriteLocker scopedMarkersLocker(&_animationMarkersLock);

    _animationMarkers.clear();
}

bool OsmAnd::MapMarkersAnimator_P::update(const float timePassed)
{
    if (_isPaused)
        return true;

    QWriteLocker scopedLocker(&_animationsCollectionLock);

    // Perform all animations
    auto itAnimations = mutableIteratorOf(_animationsByKey);
    int pausedAnimationsCount = 0;
    while(itAnimations.hasNext())
    {
        auto item = itAnimations.next();
        auto& key = item.key();
        auto& animations = item.value();

        {
            auto itAnimation = mutableIteratorOf(animations);
            while (itAnimation.hasNext())
            {
                const auto& animation = itAnimation.next();
                bool paused = animation->isPaused();
                if (!paused && animation->process(timePassed))
                    itAnimation.remove();

                if (paused)
                    pausedAnimationsCount++;
            }
        }
        
        if (animations.isEmpty())
        {
            itAnimations.remove();

            QWriteLocker scopedMarkersLocker(&_animationMarkersLock);

            const auto itMarker = _animationMarkers.find(key);
            if (itMarker != _animationMarkers.end())
                _animationMarkers.erase(itMarker);
        }
    }
    // Return true to indicate that processing has finished
    int animationsCount = _animationsByKey.size();
    return animationsCount == 0 || animationsCount == pausedAnimationsCount;
}

void OsmAnd::MapMarkersAnimator_P::animatePositionBy(
    const std::shared_ptr<MapMarker> mapMarker,
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructPositionAnimationByDelta(newAnimations, mapMarker, deltaValue, duration, timingFunction);

    QWriteLocker scopedMarkersLocker(&_animationMarkersLock);

    Key key = mapMarker.get();
    _animationMarkers[key] = std::weak_ptr<MapMarker>(mapMarker);
    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapMarkersAnimator_P::animatePositionTo(
    const std::shared_ptr<MapMarker> mapMarker,
    const PointI& value,
    const float duration,
    const TimingFunction timingFunction)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructPositionAnimationToValue(newAnimations, mapMarker, value, duration, timingFunction);

    QWriteLocker scopedMarkersLocker(&_animationMarkersLock);

    Key key = mapMarker.get();
    _animationMarkers[key] = std::weak_ptr<MapMarker>(mapMarker);
    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapMarkersAnimator_P::animatePositionWith(
    const std::shared_ptr<MapMarker> mapMarker,
    const PointD& velocity,
    const PointD& deceleration)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    animatePositionBy(mapMarker, deltaValue, duration, TimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapMarkersAnimator_P::animateDirectionBy(
    const std::shared_ptr<MapMarker> mapMarker,
    const OnSurfaceIconKey iconKey,
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructDirectionAnimationByDelta(newAnimations, mapMarker, iconKey, deltaValue, duration, timingFunction);

    QWriteLocker scopedMarkersLocker(&_animationMarkersLock);

    Key key = mapMarker.get();
    _animationMarkers[key] = std::weak_ptr<MapMarker>(mapMarker);
    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapMarkersAnimator_P::animateDirectionTo(
    const std::shared_ptr<MapMarker> mapMarker,
    const OnSurfaceIconKey iconKey,
    const float value,
    const float duration,
    const TimingFunction timingFunction)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructDirectionAnimationToValue(newAnimations, mapMarker, iconKey, value, duration, timingFunction);

    QWriteLocker scopedMarkersLocker(&_animationMarkersLock);

    Key key = mapMarker.get();
    _animationMarkers[key] = std::weak_ptr<MapMarker>(mapMarker);
    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapMarkersAnimator_P::animateDirectionWith(
    const std::shared_ptr<MapMarker> mapMarker,
    const OnSurfaceIconKey iconKey,
    const float velocity,
    const float deceleration)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateDirectionBy(mapMarker, iconKey, deltaValue, duration, TimingFunction::EaseOutQuadratic);
}

float OsmAnd::MapMarkersAnimator_P::directionGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    QReadLocker scopedLocker(&_animationMarkersLock);

    const auto itMarker = _animationMarkers.find(key);
    if (itMarker == _animationMarkers.end() || sharedContext->storageList.empty())
        return Q_SNAN;

    auto& weakMarker = *itMarker;
    auto marker = weakMarker.lock();

    OnSurfaceIconKey iconKey = sharedContext->storageList.first().value<void*>();
    return marker->getOnMapSurfaceIconDirection(iconKey);
}

void OsmAnd::MapMarkersAnimator_P::directionSetter(const Key key, const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    QReadLocker scopedLocker(&_animationMarkersLock);

    const auto itMarker = _animationMarkers.find(key);
    if (itMarker == _animationMarkers.end() || sharedContext->storageList.empty())
        return;

    auto& weakMarker = *itMarker;
    auto marker = weakMarker.lock();

    OnSurfaceIconKey iconKey = sharedContext->storageList.first().value<void*>();
    return marker->setOnMapSurfaceIconDirection(iconKey, newValue);
}

OsmAnd::PointI64 OsmAnd::MapMarkersAnimator_P::positionGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    QReadLocker scopedLocker(&_animationMarkersLock);

    const auto itMarker = _animationMarkers.find(key);
    if (itMarker == _animationMarkers.end())
        return PointI64();

    auto& weakMarker = *itMarker;
    auto marker = weakMarker.lock();

    return marker->getPosition();
}

void OsmAnd::MapMarkersAnimator_P::positionSetter(const Key key, const PointI64 newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    QReadLocker scopedLocker(&_animationMarkersLock);

    const auto itMarker = _animationMarkers.find(key);
    if (itMarker == _animationMarkers.end())
        return;

    auto& weakMarker = *itMarker;
    auto marker = weakMarker.lock();

    marker->setPosition(Utilities::normalizeCoordinates(newValue, ZoomLevel31));
}

void OsmAnd::MapMarkersAnimator_P::constructPositionAnimationByDelta(
    AnimationsCollection& outAnimation,
    const std::shared_ptr<MapMarker> mapMarker,
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || (deltaValue.x == 0 && deltaValue.y == 0))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<PointI64>(
        mapMarker.get(),
        AnimatedValue::Target,
        deltaValue, duration, 0.0f, timingFunction,
        _positionGetter, _positionSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapMarkersAnimator_P::constructPositionAnimationToValue(
    AnimationsCollection& outAnimation,
    const std::shared_ptr<MapMarker> mapMarker,
    const PointI& value,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<PointI64>(
        mapMarker.get(),
        AnimatedValue::Target,
        [this, value]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> PointI64
        {
            return PointI64(value) - PointI64(positionGetter(key, context, sharedContext));
        },
        duration, 0.0f, timingFunction,
        _positionGetter, _positionSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapMarkersAnimator_P::constructDirectionAnimationByDelta(
    AnimationsCollection& outAnimation,
    const std::shared_ptr<MapMarker> mapMarker,
    const OnSurfaceIconKey iconKey,
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || qFuzzyIsNull(deltaValue))
        return;

    const std::shared_ptr<AnimationContext> sharedContext(new AnimationContext());
    sharedContext->storageList.append(QVariant::fromValue(const_cast<void*>(iconKey)));
    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        mapMarker.get(),
        AnimatedValue::Azimuth,
        deltaValue, duration, 0.0f, timingFunction,
        _directionGetter, _directionSetter, sharedContext));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapMarkersAnimator_P::constructDirectionAnimationToValue(
    AnimationsCollection& outAnimation,
    const std::shared_ptr<MapMarker> mapMarker,
    const OnSurfaceIconKey iconKey,
    const float value,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    const std::shared_ptr<AnimationContext> sharedContext(new AnimationContext());
    sharedContext->storageList.append(QVariant::fromValue(const_cast<void*>(iconKey)));
    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        mapMarker.get(),
        AnimatedValue::Azimuth,
        [this, value]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return Utilities::normalizedAngleDegrees(value - directionGetter(key, context, sharedContext));
        },
        duration, 0.0f, timingFunction,
        _directionGetter, _directionSetter, sharedContext));

    outAnimation.push_back(qMove(newAnimation));
}

std::shared_ptr<OsmAnd::GenericAnimation> OsmAnd::MapMarkersAnimator_P::findCurrentAnimation(const AnimatedValue animatedValue, const AnimationsCollection& collection)
{
    for (const auto& animation : constOf(collection))
    {
        if (animation->getAnimatedValue() != animatedValue || !animation->isPlaying())
            continue;

        return animation;
    }

    return nullptr;
}
