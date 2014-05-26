#include "MapAnimator_P.h"
#include "MapAnimator.h"

#include "IMapRenderer.h"
#include "Utilities.h"

OsmAnd::MapAnimator_P::MapAnimator_P( MapAnimator* const owner_ )
    : owner(owner_)
    , _isAnimationPaused(true)
    , _animationsMutex(QMutex::Recursive)
    , _zoomGetter(std::bind(&MapAnimator_P::zoomGetter, this, std::placeholders::_1, std::placeholders::_2))
    , _zoomSetter(std::bind(&MapAnimator_P::zoomSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _azimuthGetter(std::bind(&MapAnimator_P::azimuthGetter, this, std::placeholders::_1, std::placeholders::_2))
    , _azimuthSetter(std::bind(&MapAnimator_P::azimuthSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _elevationAngleGetter(std::bind(&MapAnimator_P::elevationAngleGetter, this, std::placeholders::_1, std::placeholders::_2))
    , _elevationAngleSetter(std::bind(&MapAnimator_P::elevationAngleSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _targetGetter(std::bind(&MapAnimator_P::targetGetter, this, std::placeholders::_1, std::placeholders::_2))
    , _targetSetter(std::bind(&MapAnimator_P::targetSetter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
{
}

OsmAnd::MapAnimator_P::~MapAnimator_P()
{
}

void OsmAnd::MapAnimator_P::setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    cancelAnimation();
    _renderer = mapRenderer;
}

bool OsmAnd::MapAnimator_P::isAnimationPaused() const
{
    return _isAnimationPaused;
}

bool OsmAnd::MapAnimator_P::isAnimationRunning() const
{
    return !_isAnimationPaused && !_animations.isEmpty();
}

void OsmAnd::MapAnimator_P::pauseAnimation()
{
    _isAnimationPaused = true;
}

void OsmAnd::MapAnimator_P::resumeAnimation()
{
    _isAnimationPaused = false;
}

void OsmAnd::MapAnimator_P::cancelAnimation()
{
    QMutexLocker scopedLocker(&_animationsMutex);

    _isAnimationPaused = true;
    _animations.clear();
}

QList< std::shared_ptr<const OsmAnd::MapAnimator_P::IAnimation> > OsmAnd::MapAnimator_P::getAnimations() const
{
    QMutexLocker scopedLocker(&_animationsMutex);

    QList< std::shared_ptr<const IAnimation> > animations;
    animations.reserve(_animations.count());
    for (const auto& animation : constOf(_animations))
        animations.push_back(std::static_pointer_cast<const IAnimation>(animation));

    return animations;
}

std::shared_ptr<const OsmAnd::MapAnimator_P::IAnimation> OsmAnd::MapAnimator_P::getCurrentAnimationOf(const AnimatedValue value) const
{
    QMutexLocker scopedLocker(&_animationsMutex);

    for (const auto& animation : constOf(_animations))
    {
        if (animation->animatedValue != value || !animation->isActive())
            continue;

        return std::static_pointer_cast<const IAnimation>(animation);
    }

    return nullptr;
}

void OsmAnd::MapAnimator_P::update(const float timePassed)
{
    // Do nothing if animation is paused
    if (_isAnimationPaused)
        return;

    // Apply all animations
    QMutexLocker scopedLocker(&_animationsMutex);
    QMutableListIterator< std::shared_ptr<GenericAnimation> > itAnimation(_animations);
    while(itAnimation.hasNext())
    {
        const auto& animation = itAnimation.next();

        if (animation->process(timePassed))
            itAnimation.remove();
    }
}

void OsmAnd::MapAnimator_P::animateZoomBy(const float deltaValue, const float duration, const TimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructZoomAnimationByDelta(_animations, deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateZoomTo(const float value, const float duration, const TimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructZoomAnimationToValue(_animations, value, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateZoomWith(const float velocity, const float deceleration)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateZoomBy(deltaValue, duration, TimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::animateTargetBy(const PointI& deltaValue, const float duration, const TimingFunction timingFunction)
{
    animateTargetBy(PointI64(deltaValue), duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateTargetBy(const PointI64& deltaValue, const float duration, const TimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructTargetAnimationByDelta(_animations, deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateTargetTo(const PointI& value, const float duration, const TimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructTargetAnimationToValue(_animations, value, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateTargetWith(const PointD& velocity, const PointD& deceleration)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    animateTargetBy(deltaValue, duration, TimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetBy(const PointI& deltaValue, const float duration, const TimingFunction targetTimingFunction, const TimingFunction zoomTimingFunction)
{
    parabolicAnimateTargetBy(PointI64(deltaValue), duration, targetTimingFunction, zoomTimingFunction);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetBy(const PointI64& deltaValue, const float duration, const TimingFunction targetTimingFunction, const TimingFunction zoomTimingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructParabolicTargetAnimationByDelta(_animations, deltaValue, duration, targetTimingFunction, zoomTimingFunction);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetTo(const PointI& value, const float duration, const TimingFunction targetTimingFunction, const TimingFunction zoomTimingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructParabolicTargetAnimationToValue(_animations, value, duration, targetTimingFunction, zoomTimingFunction);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetWith(const PointD& velocity, const PointD& deceleration)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    parabolicAnimateTargetBy(deltaValue, duration, TimingFunction::EaseOutQuadratic, TimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::animateAzimuthBy(const float deltaValue, const float duration, const TimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructAzimuthAnimationByDelta(_animations, deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateAzimuthTo(const float value, const float duration, const TimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructAzimuthAnimationToValue(_animations, value, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateAzimuthWith(const float velocity, const float deceleration)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateAzimuthBy(deltaValue, duration, TimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::animateElevationAngleBy(const float deltaValue, const float duration, const TimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);
    
    constructElevationAngleAnimationByDelta(_animations, deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateElevationAngleTo(const float value, const float duration, const TimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructElevationAngleAnimationToValue(_animations, value, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateElevationAngleWith(const float velocity, const float deceleration)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateElevationAngleBy(deltaValue, duration, TimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::animateMoveBy(
    const PointI& deltaValue, const float duration,
    const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
    const TimingFunction timingFunction)
{
    animateMoveBy(PointI64(deltaValue), duration, zeroizeAzimuth, invZeroizeElevationAngle, timingFunction);
}

void OsmAnd::MapAnimator_P::animateMoveBy(
    const PointI64& deltaValue, const float duration,
    const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
    const TimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructParabolicTargetAnimationByDelta(_animations, deltaValue, duration, timingFunction, TimingFunction::EaseOutInQuadratic);
    if (zeroizeAzimuth)
        constructZeroizeAzimuthAnimation(_animations, duration, timingFunction);
    if (invZeroizeElevationAngle)
        constructInvZeroizeElevationAngleAnimation(_animations, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateMoveTo(
    const PointI& value, const float duration,
    const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
    const TimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructParabolicTargetAnimationToValue(_animations, value, duration, timingFunction, TimingFunction::EaseOutInQuadratic);
    if (zeroizeAzimuth)
        constructZeroizeAzimuthAnimation(_animations, duration, timingFunction);
    if (invZeroizeElevationAngle)
        constructInvZeroizeElevationAngleAnimation(_animations, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateMoveWith(const PointD& velocity, const PointD& deceleration, const bool zeroizeAzimuth, const bool invZeroizeElevationAngle)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    animateMoveBy(deltaValue, duration, zeroizeAzimuth, invZeroizeElevationAngle, TimingFunction::EaseOutQuadratic);
}

float OsmAnd::MapAnimator_P::zoomGetter(AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    return _renderer->state.requestedZoom;
}

void OsmAnd::MapAnimator_P::zoomSetter(const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setZoom(newValue);
}

float OsmAnd::MapAnimator_P::azimuthGetter(AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    return _renderer->state.azimuth;
}

void OsmAnd::MapAnimator_P::azimuthSetter(const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setAzimuth(newValue);
}

float OsmAnd::MapAnimator_P::elevationAngleGetter(AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    return _renderer->state.elevationAngle;
}

void OsmAnd::MapAnimator_P::elevationAngleSetter(const float newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setElevationAngle(newValue);
}

OsmAnd::PointI64 OsmAnd::MapAnimator_P::targetGetter(AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    return _renderer->state.target31;
}

void OsmAnd::MapAnimator_P::targetSetter(const PointI64 newValue, AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
{
    _renderer->setTarget(Utilities::normalizeCoordinates(newValue, ZoomLevel31));
}

void OsmAnd::MapAnimator_P::constructZoomAnimationByDelta(
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || qFuzzyIsNull(deltaValue))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        AnimatedValue::Zoom,
        deltaValue, duration, 0.0f, timingFunction,
        _zoomGetter, _zoomSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructZoomAnimationToValue(
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const float value,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
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
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || (deltaValue.x == 0 && deltaValue.y == 0))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<PointI64>(
        AnimatedValue::Target,
        deltaValue, duration, 0.0f, timingFunction,
        _targetGetter, _targetSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructTargetAnimationToValue(
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const PointI& value,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<PointI64>(
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
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction targetTimingFunction,
    const TimingFunction zoomTimingFunction)
{
    if (qFuzzyIsNull(duration) || (deltaValue.x == 0 && deltaValue.y == 0))
        return;

    constructTargetAnimationByDelta(outAnimation, deltaValue, duration, targetTimingFunction);
    constructParabolicTargetAnimation_Zoom(outAnimation, duration, zoomTimingFunction);
}

void OsmAnd::MapAnimator_P::constructParabolicTargetAnimationToValue(
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const PointI& value,
    const float duration,
    const TimingFunction targetTimingFunction,
    const TimingFunction zoomTimingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    constructTargetAnimationToValue(outAnimation, value, duration, targetTimingFunction);
    constructParabolicTargetAnimation_Zoom(outAnimation, duration, zoomTimingFunction);
}

void OsmAnd::MapAnimator_P::constructParabolicTargetAnimation_Zoom(
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const float duration,
    const TimingFunction zoomTimingFunction)
{
    const auto halfDuration = duration / 2.0f;

    const auto targetAnimation = outAnimation.last();

    const std::shared_ptr<AnimationContext> sharedContext(new AnimationContext());
    std::shared_ptr<GenericAnimation> zoomOutAnimation(new MapAnimator_P::Animation<float>(
        AnimatedValue::Zoom,
        [this, targetAnimation]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
        {
            PointI64 targetDeltaValue;
            bool ok = targetAnimation->obtainDeltaValueAsPointI64(targetDeltaValue);
            assert(ok);

            // Recalculate delta to tiles at current zoom base
            PointI64 deltaInTiles;
            const auto bitShift = MaxZoomLevel - _renderer->state.zoomBase;
            deltaInTiles.x = qAbs(targetDeltaValue.x) >> bitShift;
            deltaInTiles.y = qAbs(targetDeltaValue.y) >> bitShift;

            // Calculate distance in unscaled visible tiles
            const auto distance = deltaInTiles.norm();

            // Get current zoom
            const auto currentZoom = zoomGetter(context, sharedContext);
            const auto minZoom = _renderer->getMinZoom();

            // Calculate zoom shift
            float zoomShift = (std::log10(distance) - 1.3f /*~= std::log10f(20.0f)*/) * 7.0f;
            if (zoomShift <= 0.0f)
                return 0.0f;

            // If zoom shift will move zoom out of bounds, reduce zoom shift
            if (currentZoom - zoomShift < minZoom)
                zoomShift = currentZoom - minZoom;

            sharedContext->storageList.push_back(QVariant(zoomShift));
            return -zoomShift;
        },
        halfDuration, 0.0f, zoomTimingFunction,
        _zoomGetter, _zoomSetter, sharedContext));
    std::shared_ptr<GenericAnimation> zoomInAnimation(new MapAnimator_P::Animation<float>(
        AnimatedValue::Zoom,
        [this]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
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
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || qFuzzyIsNull(deltaValue))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        AnimatedValue::Azimuth,
        deltaValue, duration, 0.0f, timingFunction,
        _azimuthGetter, _azimuthSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructAzimuthAnimationToValue(
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const float value,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        AnimatedValue::Azimuth,
        [this, value]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext) -> float
        {
            return value - azimuthGetter(context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _azimuthGetter, _azimuthSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructElevationAngleAnimationByDelta(
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || qFuzzyIsNull(deltaValue))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        AnimatedValue::ElevationAngle,
        deltaValue, duration, 0.0f, timingFunction,
        _elevationAngleGetter, _elevationAngleSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructElevationAngleAnimationToValue(
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const float value,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
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
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        AnimatedValue::Azimuth,
        [this]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
        {
            return -azimuthGetter(context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _azimuthGetter, _azimuthSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructInvZeroizeElevationAngleAnimation(
    QList< std::shared_ptr<GenericAnimation> >& outAnimation,
    const float duration,
    const TimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<GenericAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        AnimatedValue::ElevationAngle,
        [this]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
        {
            return 90.0f - elevationAngleGetter(context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _elevationAngleGetter, _elevationAngleSetter));

    outAnimation.push_back(qMove(newAnimation));
}

OsmAnd::MapAnimator_P::GenericAnimation::GenericAnimation(
    const AnimatedValue animatedValue_,
    const float duration_,
    const float delay_,
    const TimingFunction timingFunction_,
    const std::shared_ptr<AnimationContext>& sharedContext_)
    : _timePassed(0.0f)
    , _sharedContext(sharedContext_)
    , animatedValue(animatedValue_)
    , duration(duration_)
    , delay(delay_)
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

OsmAnd::MapAnimator_P::AnimatedValue OsmAnd::MapAnimator_P::GenericAnimation::getAnimatedValue() const
{
    return animatedValue;
}

bool OsmAnd::MapAnimator_P::GenericAnimation::isActive() const
{
    QReadLocker scopedLock(&_processLock);

    return (_timePassed >= delay) && ((_timePassed - delay) < duration);
}

float OsmAnd::MapAnimator_P::GenericAnimation::getTimePassed() const
{
    QReadLocker scopedLock(&_processLock);

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
