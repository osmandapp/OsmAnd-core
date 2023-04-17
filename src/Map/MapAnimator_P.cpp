#include "MapAnimator_P.h"
#include "MapAnimator.h"

#include "QtCommon.h"

#include "IMapRenderer.h"
#include "Utilities.h"

OsmAnd::MapAnimator_P::MapAnimator_P( MapAnimator* const owner_ )
    : _rendererSymbolsUpdateSuspended(false)
    , _isPaused(true)
    , _zoomGetter(std::bind(&MapAnimator_P::zoomGetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _zoomSetter(std::bind(&MapAnimator_P::zoomSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
    , _azimuthGetter(std::bind(&MapAnimator_P::azimuthGetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _azimuthSetter(std::bind(&MapAnimator_P::azimuthSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
    , _elevationAngleGetter(std::bind(&MapAnimator_P::elevationAngleGetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _elevationAngleSetter(std::bind(&MapAnimator_P::elevationAngleSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
    , _targetGetter(std::bind(&MapAnimator_P::targetGetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _targetSetter(std::bind(&MapAnimator_P::targetSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
    , _flatTargetGetter(std::bind(&MapAnimator_P::flatTargetGetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _flatTargetSetter(std::bind(&MapAnimator_P::flatTargetSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
    , owner(owner_)
{
}

OsmAnd::MapAnimator_P::~MapAnimator_P()
{
}

void OsmAnd::MapAnimator_P::setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer)
{
    QMutexLocker scopedLocker(&_updateLock);

    _isPaused = true;
    _animationsByKey.clear();
    _renderer = mapRenderer;
}

bool OsmAnd::MapAnimator_P::isPaused() const
{
    return _isPaused;
}

void OsmAnd::MapAnimator_P::pause()
{
    _isPaused = true;
}

void OsmAnd::MapAnimator_P::resume()
{
    _isPaused = false;
}

bool OsmAnd::MapAnimator_P::cancelAnimation(const std::shared_ptr<const IAnimation>& animation)
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
        _animationsByKey.erase(itAnimations);

    return wasRemoved;
}

QList< std::shared_ptr<OsmAnd::IAnimation> > OsmAnd::MapAnimator_P::getAnimations(const Key key)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return QList< std::shared_ptr<IAnimation> >();

    return copyAs< QList< std::shared_ptr<IAnimation> > >(*citAnimations);
}

QList< std::shared_ptr<const OsmAnd::IAnimation> > OsmAnd::MapAnimator_P::getAnimations(const Key key) const
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return QList< std::shared_ptr<const IAnimation> >();

    return copyAs< QList< std::shared_ptr<const IAnimation> > >(*citAnimations);
}

bool OsmAnd::MapAnimator_P::pauseAnimations(const Key key)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return false;
    const auto& animations = *citAnimations;

    for (const auto& animation : constOf(animations))
        animation->pause();

    return true;
}

bool OsmAnd::MapAnimator_P::resumeAnimations(const Key key)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return false;
    const auto& animations = *citAnimations;

    for (const auto& animation : constOf(animations))
        animation->resume();

    return true;
}

bool OsmAnd::MapAnimator_P::cancelAnimations(const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    const auto itAnimations = _animationsByKey.find(key);
    if (itAnimations == _animationsByKey.end())
        return false;

    _animationsByKey.erase(itAnimations);

    return true;
}

bool OsmAnd::MapAnimator_P::cancelCurrentAnimation(const Key key, const AnimatedValue animatedValue)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    const auto itAnimations = _animationsByKey.find(key);
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
        _animationsByKey.erase(itAnimations);

    return wasRemoved;
}

std::shared_ptr<OsmAnd::IAnimation> OsmAnd::MapAnimator_P::getCurrentAnimation(const Key key, const AnimatedValue animatedValue)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return nullptr;

    return findCurrentAnimation(animatedValue, *citAnimations);
}

std::shared_ptr<const OsmAnd::IAnimation> OsmAnd::MapAnimator_P::getCurrentAnimation(const Key key, const AnimatedValue animatedValue) const
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return nullptr;

    return findCurrentAnimation(animatedValue, *citAnimations);
}

QList< std::shared_ptr<OsmAnd::IAnimation> > OsmAnd::MapAnimator_P::getAllAnimations()
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    QList< std::shared_ptr<IAnimation> > result;
    for (const auto& animations : constOf(_animationsByKey))
        for (const auto& animation : constOf(animations))
            result.push_back(animation);

    return result;
}

QList< std::shared_ptr<const OsmAnd::IAnimation> > OsmAnd::MapAnimator_P::getAllAnimations() const
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    QList< std::shared_ptr<const IAnimation> > result;
    for (const auto& animations : constOf(_animationsByKey))
        for (const auto& animation : constOf(animations))
            result.push_back(animation);

    return result;
}

void OsmAnd::MapAnimator_P::cancelAllAnimations()
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    _animationsByKey.clear();
}

bool OsmAnd::MapAnimator_P::update(const float timePassed)
{
    if (_isPaused)
    {
        if (owner->suspendSymbolsDuringAnimation && _rendererSymbolsUpdateSuspended)
        {
            _renderer->resumeSymbolsUpdate();
            _rendererSymbolsUpdateSuspended = false;
        }
        return true;
    }

    QWriteLocker scopedLocker(&_animationsCollectionLock);

    // In case there are animations and symbols suspend is enabled,
    // actually suspend symbols
    // In case there are no animations and symbols suspend is enabled,
    // resume symbols update if it was enabled
    if (owner->suspendSymbolsDuringAnimation)
    {
        if (!_animationsByKey.isEmpty() && !_rendererSymbolsUpdateSuspended)
        {
            _renderer->suspendSymbolsUpdate();
            _rendererSymbolsUpdateSuspended = true;
        }
        else if (_animationsByKey.isEmpty() && _rendererSymbolsUpdateSuspended)
        {
            _renderer->resumeSymbolsUpdate();
            _rendererSymbolsUpdateSuspended = false;
        }
    }

    // Perform all animations
    auto itAnimations = mutableIteratorOf(_animationsByKey);
    int pausedAnimationsCount = 0;
    while(itAnimations.hasNext())
    {
        auto& animations = itAnimations.next().value();

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
            itAnimations.remove();
    }
    // Return true to indicate that processing has finished
    int animationsCount = _animationsByKey.size();
    return animationsCount == 0 || animationsCount == pausedAnimationsCount;
}

void OsmAnd::MapAnimator_P::animateZoomBy(
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructZoomAnimationByDelta(newAnimations, key, deltaValue, duration, timingFunction);

    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateZoomTo(
    const float value,
    const float duration,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructZoomAnimationToValue(newAnimations, key, value, duration, timingFunction);

    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateZoomWith(
    const float velocity,
    const float deceleration,
    const Key key)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateZoomBy(deltaValue, duration, TimingFunction::EaseOutQuadratic, key);
}

void OsmAnd::MapAnimator_P::animateZoomToAndPan(
    const float value,
    const PointI& panValue,
    const float duration,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructZoomAnimationToValueAndPan(newAnimations, key, value, panValue, duration, timingFunction);

    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateTargetBy(
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructTargetAnimationByDelta(newAnimations, key, deltaValue, duration, timingFunction);

    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateTargetTo(
    const PointI& value,
    const float duration,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructTargetAnimationToValue(newAnimations, key, value, duration, timingFunction);

    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateTargetWith(
    const PointD& velocity,
    const PointD& deceleration,
    const Key key)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    animateTargetBy(deltaValue, duration, TimingFunction::EaseOutQuadratic, key);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetBy(
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction targetTimingFunction,
    const TimingFunction zoomTimingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructParabolicTargetAnimationByDelta(newAnimations, key, deltaValue, duration, targetTimingFunction, zoomTimingFunction);

    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetTo(
    const PointI& value,
    const float duration,
    const TimingFunction targetTimingFunction,
    const TimingFunction zoomTimingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructParabolicTargetAnimationToValue(newAnimations, key, value, duration, targetTimingFunction, zoomTimingFunction);

    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetWith(
    const PointD& velocity,
    const PointD& deceleration,
    const Key key)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    parabolicAnimateTargetBy(deltaValue, duration, TimingFunction::EaseOutQuadratic, TimingFunction::EaseOutQuadratic, key);
}

void OsmAnd::MapAnimator_P::animateFlatTargetWith(
    const PointD& velocity,
    const PointD& deceleration,
    const Key key)
{
    const auto duration = qSqrt((velocity.x * velocity.x + velocity.y * velocity.y) /
        (deceleration.x * deceleration.x + deceleration.y * deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructFlatTargetAnimationByDelta(newAnimations, key, deltaValue, duration, TimingFunction::EaseOutQuadratic);

    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateAzimuthBy(
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructAzimuthAnimationByDelta(newAnimations, key, deltaValue, duration, timingFunction);
    
    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateAzimuthTo(
    const float value,
    const float duration,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructAzimuthAnimationToValue(newAnimations, key, value, duration, timingFunction);
    
    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateAzimuthWith(
    const float velocity,
    const float deceleration,
    const Key key)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateAzimuthBy(deltaValue, duration, TimingFunction::EaseOutQuadratic, key);
}

void OsmAnd::MapAnimator_P::animateElevationAngleBy(
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);
    
    AnimationsCollection newAnimations;
    constructElevationAngleAnimationByDelta(newAnimations, key, deltaValue, duration, timingFunction);
    
    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateElevationAngleTo(
    const float value,
    const float duration,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructElevationAngleAnimationToValue(newAnimations, key, value, duration, timingFunction);
    
    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateElevationAngleWith(
    const float velocity,
    const float deceleration,
    const Key key)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateElevationAngleBy(deltaValue, duration, TimingFunction::EaseOutQuadratic, key);
}

void OsmAnd::MapAnimator_P::animateMoveBy(
    const PointI64& deltaValue,
    const float duration,
    const bool zeroizeAzimuth,
    const bool invZeroizeElevationAngle,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructParabolicTargetAnimationByDelta(newAnimations, key, deltaValue, duration, timingFunction, TimingFunction::EaseOutInQuadratic);
    if (zeroizeAzimuth)
        constructZeroizeAzimuthAnimation(newAnimations, key, duration, timingFunction);
    if (invZeroizeElevationAngle)
        constructInvZeroizeElevationAngleAnimation(newAnimations, key, duration, timingFunction);
    
    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateMoveTo(
    const PointI& value,
    const float duration,
    const bool zeroizeAzimuth,
    const bool invZeroizeElevationAngle,
    const TimingFunction timingFunction,
    const Key key)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    AnimationsCollection newAnimations;
    constructParabolicTargetAnimationToValue(newAnimations, key, value, duration, timingFunction, TimingFunction::EaseOutInQuadratic);
    if (zeroizeAzimuth)
        constructZeroizeAzimuthAnimation(newAnimations, key, duration, timingFunction);
    if (invZeroizeElevationAngle)
        constructInvZeroizeElevationAngleAnimation(newAnimations, key, duration, timingFunction);

    _animationsByKey[key].append(newAnimations);
}

void OsmAnd::MapAnimator_P::animateMoveWith(
    const PointD& velocity,
    const PointD& deceleration,
    const bool zeroizeAzimuth,
    const bool invZeroizeElevationAngle,
    const Key key)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    animateMoveBy(deltaValue, duration, zeroizeAzimuth, invZeroizeElevationAngle, TimingFunction::EaseOutQuadratic, key);
}

float OsmAnd::MapAnimator_P::zoomGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    const auto state = _renderer->getState();
    const auto zoom = state.zoomLevel + (state.visualZoom >= 1.0f ? state.visualZoom - 1.0f : (state.visualZoom - 1.0f) * 2.0f);
    return zoom;
}

void OsmAnd::MapAnimator_P::zoomSetter(const Key key, const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setZoom(newValue);
}

float OsmAnd::MapAnimator_P::azimuthGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    return _renderer->getState().azimuth;
}

void OsmAnd::MapAnimator_P::azimuthSetter(const Key key, const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setAzimuth(newValue);
}

float OsmAnd::MapAnimator_P::elevationAngleGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    return _renderer->getState().elevationAngle;
}

void OsmAnd::MapAnimator_P::elevationAngleSetter(const Key key, const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setElevationAngle(newValue);
}

OsmAnd::PointI64 OsmAnd::MapAnimator_P::targetGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    PointI location31;
    _renderer->getMapTargetLocation(location31);
    return location31;
}

void OsmAnd::MapAnimator_P::targetSetter(const Key key, const PointI64 newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setMapTargetLocation(Utilities::normalizeCoordinates(newValue, ZoomLevel31));
    //_renderer->setTarget(Utilities::normalizeCoordinates(newValue, ZoomLevel31));
    _renderer->targetChangedObservable.postNotify(_renderer.get());
}

OsmAnd::PointI64 OsmAnd::MapAnimator_P::flatTargetGetter(const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    return _renderer->getState().target31;
}

void OsmAnd::MapAnimator_P::flatTargetSetter(const Key key, const PointI64 newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setTarget(Utilities::normalizeCoordinates(newValue, ZoomLevel31));
    _renderer->targetChangedObservable.postNotify(_renderer.get());
}

void OsmAnd::MapAnimator_P::constructZoomAnimationByDelta(
    AnimationsCollection& outAnimation,
    const Key key,
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || qFuzzyIsNull(deltaValue))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        key,
        AnimatedValue::Zoom,
        deltaValue, duration, 0.0f, timingFunction,
        _zoomGetter, _zoomSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructZoomAnimationToValue(
    AnimationsCollection& outAnimation,
    const Key key,
    const float value,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        key,
        AnimatedValue::Zoom,
        [this, value]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return value - zoomGetter(key, context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _zoomGetter, _zoomSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructZoomAnimationToValueAndPan(
    AnimationsCollection& outAnimation,
    const Key key,
    const float value,
    const PointI& panValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        key,
        AnimatedValue::Zoom,
        [this, value]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return value - zoomGetter(key, context, sharedContext);
        },
        [this, panValue]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> PointI64
        {
            return PointI64(panValue) - PointI64(targetGetter(key, context, sharedContext));
        },
        duration, 0.0f, timingFunction,
        _zoomGetter, _zoomSetter, _targetGetter, _targetSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructFlatTargetAnimationByDelta(
    AnimationsCollection& outAnimation,
    const Key key,
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || (deltaValue.x == 0 && deltaValue.y == 0))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<PointI64>(
        key,
        AnimatedValue::Target,
        deltaValue, duration, 0.0f, timingFunction,
        _flatTargetGetter, _flatTargetSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructTargetAnimationByDelta(
    AnimationsCollection& outAnimation,
    const Key key,
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || (deltaValue.x == 0 && deltaValue.y == 0))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<PointI64>(
        key,
        AnimatedValue::Target,
        deltaValue, duration, 0.0f, timingFunction,
        _targetGetter, _targetSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructTargetAnimationToValue(
    AnimationsCollection& outAnimation,
    const Key key,
    const PointI& value,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<PointI64>(
        key,
        AnimatedValue::Target,
        [this, value]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> PointI64
        {
            return PointI64(value) - PointI64(targetGetter(key, context, sharedContext));
        },
        duration, 0.0f, timingFunction,
        _targetGetter, _targetSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructParabolicTargetAnimationByDelta(
    AnimationsCollection& outAnimation,
    const Key key,
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction targetTimingFunction,
    const TimingFunction zoomTimingFunction)
{
    if (qFuzzyIsNull(duration) || (deltaValue.x == 0 && deltaValue.y == 0))
        return;

    constructTargetAnimationByDelta(outAnimation, key, deltaValue, duration, targetTimingFunction);
    constructParabolicTargetAnimationByDelta_Zoom(outAnimation, key, deltaValue, duration, zoomTimingFunction);
}

void OsmAnd::MapAnimator_P::constructParabolicTargetAnimationByDelta_Zoom(
    AnimationsCollection& outAnimation,
    const Key key,
    const PointI64& targetDeltaValue,
    const float duration,
    const TimingFunction zoomTimingFunction)
{
    const auto halfDuration = duration / 2.0f;

    const std::shared_ptr<AnimationContext> sharedContext(new AnimationContext());
    std::shared_ptr<GenericAnimation> zoomOutAnimation(new Animation<float>(
        key,
        AnimatedValue::Zoom,
        [this, targetDeltaValue]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            // Recalculate delta to tiles at current zoom base
            PointI64 deltaInTiles;
            const auto bitShift = MaxZoomLevel - _renderer->getState().zoomLevel;
            deltaInTiles.x = qAbs(targetDeltaValue.x) >> bitShift;
            deltaInTiles.y = qAbs(targetDeltaValue.y) >> bitShift;

            // Calculate distance in unscaled visible tiles
            const auto distance = deltaInTiles.norm();

            // Get current zoom
            const auto currentZoom = zoomGetter(key, context, sharedContext);
            const auto minZoomLevel = _renderer->getMaximalZoomLevelsRangeLowerBound();

            // Calculate zoom shift
            float zoomShift = (std::log10(distance) - 1.3f /*~= std::log10f(20.0f)*/) * 7.0f;
            if (zoomShift <= 0.0f)
                return 0.0f;

            // If zoom shift will move zoom out of bounds, reduce zoom shift
            if (currentZoom - zoomShift < minZoomLevel)
                zoomShift = currentZoom - minZoomLevel;

            sharedContext->storageList.push_back(QVariant(zoomShift));
            return -zoomShift;
        },
        halfDuration, 0.0f, zoomTimingFunction,
        _zoomGetter, _zoomSetter, sharedContext));
    std::shared_ptr<GenericAnimation> zoomInAnimation(new Animation<float>(
        key,
        AnimatedValue::Zoom,
        [this]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            // If shared context contains no data it means that parabolic effect was disabled
            if (sharedContext->storageList.isEmpty())
                return 0.0f;

            // Just restore the original zoom
            return sharedContext->storageList.first().toFloat();
        },
        halfDuration, halfDuration, zoomTimingFunction,
        _zoomGetter, _zoomSetter, sharedContext));

    outAnimation.push_back(qMove(zoomOutAnimation));
    outAnimation.push_back(qMove(zoomInAnimation));
}

void OsmAnd::MapAnimator_P::constructParabolicTargetAnimationToValue(
    AnimationsCollection& outAnimation,
    const Key key,
    const PointI& value,
    const float duration,
    const TimingFunction targetTimingFunction,
    const TimingFunction zoomTimingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    constructTargetAnimationToValue(outAnimation, key, value, duration, targetTimingFunction);
    constructParabolicTargetAnimationToValue_Zoom(outAnimation, key, value, duration, zoomTimingFunction);
}

void OsmAnd::MapAnimator_P::constructParabolicTargetAnimationToValue_Zoom(
    AnimationsCollection& outAnimation,
    const Key key,
    const PointI& targetValue,
    const float duration,
    const TimingFunction zoomTimingFunction)
{
    const auto halfDuration = duration / 2.0f;

    const std::shared_ptr<AnimationContext> sharedContext(new AnimationContext());
    std::shared_ptr<GenericAnimation> zoomOutAnimation(new Animation<float>(
        key,
        AnimatedValue::Zoom,
        [this, targetValue]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            PointI64 targetDeltaValue = PointI64(targetValue) - targetGetter(key, context, sharedContext);

            // Recalculate delta to tiles at current zoom base
            PointI64 deltaInTiles;
            const auto bitShift = MaxZoomLevel - _renderer->getState().zoomLevel;
            deltaInTiles.x = qAbs(targetDeltaValue.x) >> bitShift;
            deltaInTiles.y = qAbs(targetDeltaValue.y) >> bitShift;

            // Calculate distance in unscaled visible tiles
            const auto distance = deltaInTiles.norm();

            // Get current zoom
            const auto currentZoom = zoomGetter(key, context, sharedContext);
            const auto minZoomLevel = _renderer->getMaximalZoomLevelsRangeLowerBound();

            // Calculate zoom shift
            float zoomShift = (std::log10(distance) - 1.3f /*~= std::log10f(20.0f)*/) * 7.0f;
            if (zoomShift <= 0.0f)
                return 0.0f;

            // If zoom shift will move zoom out of bounds, reduce zoom shift
            if (currentZoom - zoomShift < minZoomLevel)
                zoomShift = currentZoom - minZoomLevel;

            sharedContext->storageList.push_back(QVariant(zoomShift));
            return -zoomShift;
        },
        halfDuration, 0.0f, zoomTimingFunction,
        _zoomGetter, _zoomSetter, sharedContext));
    std::shared_ptr<GenericAnimation> zoomInAnimation(new Animation<float>(
        key,
        AnimatedValue::Zoom,
        [this]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            // If shared context contains no data it means that parabolic effect was disabled
            if (sharedContext->storageList.isEmpty())
                return 0.0f;

            // Just restore the original zoom
            return sharedContext->storageList.first().toFloat();
        },
        halfDuration, halfDuration, zoomTimingFunction,
        _zoomGetter, _zoomSetter, sharedContext));

    outAnimation.push_back(qMove(zoomOutAnimation));
    outAnimation.push_back(qMove(zoomInAnimation));
}

void OsmAnd::MapAnimator_P::constructAzimuthAnimationByDelta(
    AnimationsCollection& outAnimation,
    const Key key,
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || qFuzzyIsNull(deltaValue))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        key,
        AnimatedValue::Azimuth,
        deltaValue, duration, 0.0f, timingFunction,
        _azimuthGetter, _azimuthSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructAzimuthAnimationToValue(
    AnimationsCollection& outAnimation,
    const Key key,
    const float value,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        key,
        AnimatedValue::Azimuth,
        [this, value]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return Utilities::normalizedAngleDegrees(value - azimuthGetter(key, context, sharedContext));
        },
        duration, 0.0f, timingFunction,
        _azimuthGetter, _azimuthSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructElevationAngleAnimationByDelta(
    AnimationsCollection& outAnimation,
    const Key key,
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || qFuzzyIsNull(deltaValue))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        key,
        AnimatedValue::ElevationAngle,
        deltaValue, duration, 0.0f, timingFunction,
        _elevationAngleGetter, _elevationAngleSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructElevationAngleAnimationToValue(
    AnimationsCollection& outAnimation,
    const Key key,
    const float value,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        key,
        AnimatedValue::ElevationAngle,
        [this, value]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return value - elevationAngleGetter(key, context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _elevationAngleGetter, _elevationAngleSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructZeroizeAzimuthAnimation(
    AnimationsCollection& outAnimation,
    const Key key,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        key,
        AnimatedValue::Azimuth,
        [this]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return -azimuthGetter(key, context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _azimuthGetter, _azimuthSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructInvZeroizeElevationAngleAnimation(
    AnimationsCollection& outAnimation,
    const Key key,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new Animation<float>(
        key,
        AnimatedValue::ElevationAngle,
        [this]
        (const Key key, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return 90.0f - elevationAngleGetter(key, context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _elevationAngleGetter, _elevationAngleSetter));

    outAnimation.push_back(qMove(newAnimation));
}

std::shared_ptr<OsmAnd::GenericAnimation> OsmAnd::MapAnimator_P::findCurrentAnimation(const AnimatedValue animatedValue, const AnimationsCollection& collection)
{
    for (const auto& animation : constOf(collection))
    {
        if (animation->getAnimatedValue() != animatedValue || !animation->isPlaying())
            continue;

        return animation;
    }

    return nullptr;
}
