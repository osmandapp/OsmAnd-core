#include "MapAnimator_P.h"
#include "MapAnimator.h"

#include "QtCommon.h"

#include "IMapRenderer.h"
#include "Utilities.h"

OsmAnd::MapAnimator_P::MapAnimator_P( MapAnimator* const owner_ )
    : _rendererSymbolsUpdateSuspended(false)
    , _isPaused(true)
    , _zoomGetter(std::bind(&MapAnimator_P::zoomGetter, this, std::placeholders::_1, std::placeholders::_2))
    , _zoomSetter(std::bind(&MapAnimator_P::zoomSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _azimuthGetter(std::bind(&MapAnimator_P::azimuthGetter, this, std::placeholders::_1, std::placeholders::_2))
    , _azimuthSetter(std::bind(&MapAnimator_P::azimuthSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _elevationAngleGetter(std::bind(&MapAnimator_P::elevationAngleGetter, this, std::placeholders::_1, std::placeholders::_2))
    , _elevationAngleSetter(std::bind(&MapAnimator_P::elevationAngleSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _targetGetter(std::bind(&MapAnimator_P::targetGetter, this, std::placeholders::_1, std::placeholders::_2))
    , _targetSetter(std::bind(&MapAnimator_P::targetSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
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

QList< std::shared_ptr<OsmAnd::MapAnimator_P::IAnimation> > OsmAnd::MapAnimator_P::getAnimations(const Key key)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return QList< std::shared_ptr<IAnimation> >();

    return copyAs< QList< std::shared_ptr<IAnimation> > >(*citAnimations);
}

QList< std::shared_ptr<const OsmAnd::MapAnimator_P::IAnimation> > OsmAnd::MapAnimator_P::getAnimations(const Key key) const
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

std::shared_ptr<OsmAnd::MapAnimator_P::IAnimation> OsmAnd::MapAnimator_P::getCurrentAnimation(const Key key, const AnimatedValue animatedValue)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return nullptr;

    return findCurrentAnimation(animatedValue, *citAnimations);
}

std::shared_ptr<const OsmAnd::MapAnimator_P::IAnimation> OsmAnd::MapAnimator_P::getCurrentAnimation(const Key key, const AnimatedValue animatedValue) const
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return nullptr;

    return findCurrentAnimation(animatedValue, *citAnimations);
}

QList< std::shared_ptr<OsmAnd::MapAnimator_P::IAnimation> > OsmAnd::MapAnimator_P::getAllAnimations()
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    QList< std::shared_ptr<IAnimation> > result;
    for (const auto& animations : constOf(_animationsByKey))
        for (const auto& animation : constOf(animations))
            result.push_back(animation);

    return result;
}

QList< std::shared_ptr<const OsmAnd::MapAnimator_P::IAnimation> > OsmAnd::MapAnimator_P::getAllAnimations() const
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

void OsmAnd::MapAnimator_P::update(const float timePassed)
{
    if (_isPaused)
    {
        if (owner->suspendSymbolsDuringAnimation && _rendererSymbolsUpdateSuspended)
        {
            _renderer->resumeSymbolsUpdate();
            _rendererSymbolsUpdateSuspended = false;
        }
        return;
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
    while(itAnimations.hasNext())
    {
        auto& animations = itAnimations.next().value();

        {
            auto itAnimation = mutableIteratorOf(animations);
            while (itAnimation.hasNext())
            {
                const auto& animation = itAnimation.next();

                if (!animation->isPaused() && animation->process(timePassed))
                    itAnimation.remove();
            }
        }
        
        if (animations.isEmpty())
            itAnimations.remove();
    }
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

float OsmAnd::MapAnimator_P::zoomGetter(AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    const auto state = _renderer->getState();
    const auto zoom = state.zoomLevel + (state.visualZoom >= 1.0f ? state.visualZoom - 1.0f : (state.visualZoom - 1.0f) * 2.0f);
    return zoom;
}

void OsmAnd::MapAnimator_P::zoomSetter(const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setZoom(newValue);
}

float OsmAnd::MapAnimator_P::azimuthGetter(AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    return _renderer->getState().azimuth;
}

void OsmAnd::MapAnimator_P::azimuthSetter(const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setAzimuth(newValue);
}

float OsmAnd::MapAnimator_P::elevationAngleGetter(AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    return _renderer->getState().elevationAngle;
}

void OsmAnd::MapAnimator_P::elevationAngleSetter(const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setElevationAngle(newValue);
}

OsmAnd::PointI64 OsmAnd::MapAnimator_P::targetGetter(AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    return _renderer->getState().target31;
}

void OsmAnd::MapAnimator_P::targetSetter(const PointI64 newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setTarget(Utilities::normalizeCoordinates(newValue, ZoomLevel31));
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

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
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

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        key,
        AnimatedValue::Zoom,
        [this, value]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return value - zoomGetter(context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _zoomGetter, _zoomSetter));

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

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<PointI64>(
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

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<PointI64>(
        key,
        AnimatedValue::Target,
        [this, value]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> PointI64
        {
            return PointI64(value) - PointI64(targetGetter(context, sharedContext));
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
    std::shared_ptr<GenericAnimation> zoomOutAnimation(new MapAnimator_P::Animation<float>(
        key,
        AnimatedValue::Zoom,
        [this, targetDeltaValue]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            // Recalculate delta to tiles at current zoom base
            PointI64 deltaInTiles;
            const auto bitShift = MaxZoomLevel - _renderer->getState().zoomLevel;
            deltaInTiles.x = qAbs(targetDeltaValue.x) >> bitShift;
            deltaInTiles.y = qAbs(targetDeltaValue.y) >> bitShift;

            // Calculate distance in unscaled visible tiles
            const auto distance = deltaInTiles.norm();

            // Get current zoom
            const auto currentZoom = zoomGetter(context, sharedContext);
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
    std::shared_ptr<GenericAnimation> zoomInAnimation(new MapAnimator_P::Animation<float>(
        key,
        AnimatedValue::Zoom,
        []
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
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
    std::shared_ptr<GenericAnimation> zoomOutAnimation(new MapAnimator_P::Animation<float>(
        key,
        AnimatedValue::Zoom,
        [this, targetValue]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            PointI64 targetDeltaValue = PointI64(targetValue) - targetGetter(context, sharedContext);

            // Recalculate delta to tiles at current zoom base
            PointI64 deltaInTiles;
            const auto bitShift = MaxZoomLevel - _renderer->getState().zoomLevel;
            deltaInTiles.x = qAbs(targetDeltaValue.x) >> bitShift;
            deltaInTiles.y = qAbs(targetDeltaValue.y) >> bitShift;

            // Calculate distance in unscaled visible tiles
            const auto distance = deltaInTiles.norm();

            // Get current zoom
            const auto currentZoom = zoomGetter(context, sharedContext);
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
    std::shared_ptr<GenericAnimation> zoomInAnimation(new MapAnimator_P::Animation<float>(
        key,
        AnimatedValue::Zoom,
        []
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
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

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
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

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        key,
        AnimatedValue::Azimuth,
        [this, value]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return Utilities::normalizedAngleDegrees(value - azimuthGetter(context, sharedContext));
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

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
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

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        key,
        AnimatedValue::ElevationAngle,
        [this, value]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return value - elevationAngleGetter(context, sharedContext);
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

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        key,
        AnimatedValue::Azimuth,
        [this]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return -azimuthGetter(context, sharedContext);
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

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        key,
        AnimatedValue::ElevationAngle,
        [this]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return 90.0f - elevationAngleGetter(context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _elevationAngleGetter, _elevationAngleSetter));

    outAnimation.push_back(qMove(newAnimation));
}

std::shared_ptr<OsmAnd::MapAnimator_P::GenericAnimation> OsmAnd::MapAnimator_P::findCurrentAnimation(const AnimatedValue animatedValue, const AnimationsCollection& collection)
{
    for (const auto& animation : constOf(collection))
    {
        if (animation->getAnimatedValue() != animatedValue || !animation->isPlaying())
            continue;

        return animation;
    }

    return nullptr;
}

OsmAnd::MapAnimator_P::GenericAnimation::GenericAnimation(
    const Key key_,
    const AnimatedValue animatedValue_,
    const float duration_,
    const float delay_,
    const TimingFunction timingFunction_,
    const std::shared_ptr<AnimationContext>& sharedContext_)
    : _isPaused(false)
    , _timePassed(0.0f)
    , _sharedContext(sharedContext_)
    , key(key_)
    , animatedValue(animatedValue_)
    , delay(delay_)
    , duration(duration_)
    , timingFunction(timingFunction_)
{
}

OsmAnd::MapAnimator_P::GenericAnimation::~GenericAnimation()
{
}

float OsmAnd::MapAnimator_P::GenericAnimation::properCast(const float value)
{
    return value;
}

double OsmAnd::MapAnimator_P::GenericAnimation::properCast(const double value)
{
    return value;
}

double OsmAnd::MapAnimator_P::GenericAnimation::properCast(const int32_t value)
{
    return static_cast<double>(value);
}

double OsmAnd::MapAnimator_P::GenericAnimation::properCast(const int64_t value)
{
    return static_cast<double>(value);
}

OsmAnd::MapAnimator_P::Key OsmAnd::MapAnimator_P::GenericAnimation::getKey() const
{
    return key;
}

OsmAnd::MapAnimator_P::AnimatedValue OsmAnd::MapAnimator_P::GenericAnimation::getAnimatedValue() const
{
    return animatedValue;
}

float OsmAnd::MapAnimator_P::GenericAnimation::getTimePassed() const
{
    QReadLocker scopedLocker(&_processLock);

    return _timePassed;
}

float OsmAnd::MapAnimator_P::GenericAnimation::getDelay() const
{
    return delay;
}

float OsmAnd::MapAnimator_P::GenericAnimation::getDuration() const
{
    return duration;
}

OsmAnd::MapAnimator_P::TimingFunction OsmAnd::MapAnimator_P::GenericAnimation::getTimingFunction() const
{
    return timingFunction;
}

void OsmAnd::MapAnimator_P::GenericAnimation::pause()
{
    _isPaused = true;
}

void OsmAnd::MapAnimator_P::GenericAnimation::resume()
{
    _isPaused = false;
}

bool OsmAnd::MapAnimator_P::GenericAnimation::isPaused() const
{
    return _isPaused;
}

bool OsmAnd::MapAnimator_P::GenericAnimation::isPlaying() const
{
    return (_timePassed >= delay) && ((_timePassed - delay) < duration);
}
