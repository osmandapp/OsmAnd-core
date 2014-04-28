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

void OsmAnd::MapAnimator_P::update(const float timePassed)
{
    // Do nothing if animation is paused
    if (_isAnimationPaused)
        return;

    // Apply all animations
    QMutexLocker scopedLocker(&_animationsMutex);
    QMutableListIterator< std::shared_ptr<BaseAnimation> > itAnimation(_animations);
    while(itAnimation.hasNext())
    {
        const auto& animation = itAnimation.next();

        if (animation->process(timePassed))
            itAnimation.remove();
    }
}

void OsmAnd::MapAnimator_P::animateZoomBy(const float deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructZoomAnimation(_animations, deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateZoomWith(const float velocity, const float deceleration)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateZoomBy(deltaValue, duration, MapAnimatorTimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::animateTargetBy(const PointI& deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    animateTargetBy(PointI64(deltaValue), duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateTargetBy(const PointI64& deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructTargetAnimation(_animations, deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateTargetWith(const PointD& velocity, const PointD& deceleration)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    animateTargetBy(deltaValue, duration, MapAnimatorTimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetBy(const PointI& deltaValue, const float duration, const MapAnimatorTimingFunction targetTimingFunction, const MapAnimatorTimingFunction zoomTimingFunction)
{
    parabolicAnimateTargetBy(PointI64(deltaValue), duration, targetTimingFunction, zoomTimingFunction);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetBy(const PointI64& deltaValue, const float duration, const MapAnimatorTimingFunction targetTimingFunction, const MapAnimatorTimingFunction zoomTimingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructParabolicTargetAnimation(_animations, deltaValue, duration, targetTimingFunction, zoomTimingFunction);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetWith(const PointD& velocity, const PointD& deceleration)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    parabolicAnimateTargetBy(deltaValue, duration, MapAnimatorTimingFunction::EaseOutQuadratic, MapAnimatorTimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::animateAzimuthBy(const float deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructAzimuthAnimation(_animations, deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateAzimuthWith(const float velocity, const float deceleration)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateAzimuthBy(deltaValue, duration, MapAnimatorTimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::animateElevationAngleBy(const float deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);
    
    constructElevationAngleAnimation(_animations, deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator_P::animateElevationAngleWith(const float velocity, const float deceleration)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateElevationAngleBy(deltaValue, duration, MapAnimatorTimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::animateMoveBy(
    const PointI& deltaValue, const float duration,
    const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
    const MapAnimatorTimingFunction timingFunction)
{
    animateMoveBy(PointI64(deltaValue), duration, zeroizeAzimuth, invZeroizeElevationAngle, timingFunction);
}

void OsmAnd::MapAnimator_P::animateMoveBy(
    const PointI64& deltaValue, const float duration,
    const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
    const MapAnimatorTimingFunction timingFunction)
{
    QMutexLocker scopedLocker(&_animationsMutex);

    constructParabolicTargetAnimation(_animations, deltaValue, duration, timingFunction, MapAnimatorTimingFunction::EaseOutInQuadratic);
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

    animateMoveBy(deltaValue, duration, zeroizeAzimuth, invZeroizeElevationAngle, MapAnimatorTimingFunction::EaseOutQuadratic);
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

void OsmAnd::MapAnimator_P::constructZoomAnimation(
    QList< std::shared_ptr<BaseAnimation> >& outAnimation,
    const float deltaValue,
    const float duration,
    const MapAnimatorTimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || qFuzzyIsNull(deltaValue))
        return;

    std::shared_ptr<BaseAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        deltaValue, duration, 0.0f, timingFunction,
        _zoomGetter, _zoomSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructTargetAnimation(
    QList< std::shared_ptr<BaseAnimation> >& outAnimation,
    const PointI64& deltaValue,
    const float duration,
    const MapAnimatorTimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || (deltaValue.x == 0 && deltaValue.y == 0))
        return;

    std::shared_ptr<BaseAnimation> newAnimation(new MapAnimator_P::Animation<PointI64>(
        deltaValue, duration, 0.0f, timingFunction,
        _targetGetter, _targetSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructParabolicTargetAnimation(
    QList< std::shared_ptr<BaseAnimation> >& outAnimation,
    const PointI64& deltaValue,
    const float duration,
    const MapAnimatorTimingFunction targetTimingFunction,
    const MapAnimatorTimingFunction zoomTimingFunction)
{
    if (qFuzzyIsNull(duration) || (deltaValue.x == 0 && deltaValue.y == 0))
        return;

    constructTargetAnimation(outAnimation, deltaValue, duration, targetTimingFunction);

    const auto halfDuration = duration / 2.0f;

    const std::shared_ptr<AnimationContext> sharedContext(new AnimationContext());
    std::shared_ptr<BaseAnimation> zoomOutAnimation(new MapAnimator_P::Animation<float>(
        [this, deltaValue](AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
        {
            // Recalculate delta to tiles at current zoom base
            PointI64 deltaInTiles;
            const auto bitShift = MaxZoomLevel - _renderer->state.zoomBase;
            deltaInTiles.x = qAbs(deltaValue.x) >> bitShift;
            deltaInTiles.y = qAbs(deltaValue.y) >> bitShift;

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
    std::shared_ptr<BaseAnimation> zoomInAnimation(new MapAnimator_P::Animation<float>(
            [this](AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
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

void OsmAnd::MapAnimator_P::constructAzimuthAnimation(
    QList< std::shared_ptr<BaseAnimation> >& outAnimation,
    const float deltaValue,
    const float duration,
    const MapAnimatorTimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || qFuzzyIsNull(deltaValue))
        return;

    std::shared_ptr<BaseAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        deltaValue, duration, 0.0f, timingFunction,
        _azimuthGetter, _azimuthSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructElevationAngleAnimation(
    QList< std::shared_ptr<BaseAnimation> >& outAnimation,
    const float deltaValue,
    const float duration,
    const MapAnimatorTimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration) || qFuzzyIsNull(deltaValue))
        return;

    std::shared_ptr<BaseAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        deltaValue, duration, 0.0f, timingFunction,
        _elevationAngleGetter, _elevationAngleSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructZeroizeAzimuthAnimation(
    QList< std::shared_ptr<BaseAnimation> >& outAnimation,
    const float duration,
    const MapAnimatorTimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<BaseAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        [this](AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
        {
            return -azimuthGetter(context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _azimuthGetter, _azimuthSetter));

    outAnimation.push_back(qMove(newAnimation));
}

void OsmAnd::MapAnimator_P::constructInvZeroizeElevationAngleAnimation(
    QList< std::shared_ptr<BaseAnimation> >& outAnimation,
    const float duration,
    const MapAnimatorTimingFunction timingFunction)
{
    if (qFuzzyIsNull(duration))
        return;

    std::shared_ptr<BaseAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        [this](AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
        {
            return 90.0f - elevationAngleGetter(context, sharedContext);
        },
        duration, 0.0f, timingFunction,
        _elevationAngleGetter, _elevationAngleSetter));

    outAnimation.push_back(qMove(newAnimation));
}
