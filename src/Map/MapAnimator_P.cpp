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
    if(_isAnimationPaused)
        return;

    // Apply all animations
    QMutexLocker scopedLocker(&_animationsMutex);
    QMutableListIterator< std::shared_ptr<AbstractAnimation> > itAnimation(_animations);
    while(itAnimation.hasNext())
    {
        const auto& animation = itAnimation.next();

        if(animation->process(timePassed))
            itAnimation.remove();
    }
}

void OsmAnd::MapAnimator_P::animateZoomBy(const float deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    std::shared_ptr<AbstractAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        deltaValue, duration, 0.0f, timingFunction,
        _zoomGetter, _zoomSetter));

    {
        QMutexLocker scopedLocker(&_animationsMutex);
        _animations.push_back(qMove(newAnimation));
    }
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
    std::shared_ptr<AbstractAnimation> newAnimation(new MapAnimator_P::Animation<PointI64>(
        deltaValue, duration, 0.0f, timingFunction,
        _targetGetter, _targetSetter));

    {
        QMutexLocker scopedLocker(&_animationsMutex);
        _animations.push_back(qMove(newAnimation));
    }
}

void OsmAnd::MapAnimator_P::animateTargetWith(const PointD& velocity, const PointD& deceleration)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    animateTargetBy(deltaValue, duration, MapAnimatorTimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetBy(const PointI& deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    parabolicAnimateTargetBy(PointI64(deltaValue), duration, timingFunction);
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetBy(const PointI64& deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    const auto halfDuration = duration / 2.0f;
    std::shared_ptr<AbstractAnimation> targetAnimation(new MapAnimator_P::Animation<PointI64>(
        deltaValue, duration, 0.0f, timingFunction,
        _targetGetter, _targetSetter));

    const std::shared_ptr<AnimationContext> sharedContext(new AnimationContext());
    std::shared_ptr<AbstractAnimation> zoomOutAnimation(new MapAnimator_P::Animation<float>(
        [this](AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
        {
            // Threshold for enabling parabolic effect is that
            return 0.0f;
        },
        halfDuration, 0.0f, MapAnimatorTimingFunction::EaseInQuadratic,
        _zoomGetter, _zoomSetter, sharedContext));
    std::shared_ptr<AbstractAnimation> zoomInAnimation(new MapAnimator_P::Animation<float>(
        [this](AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
        {
            // Threshold for enabling parabolic effect is that
            return 0.0f;
        },
        halfDuration, halfDuration, MapAnimatorTimingFunction::EaseOutQuadratic,
        _zoomGetter, _zoomSetter, sharedContext));

    {
        QMutexLocker scopedLocker(&_animationsMutex);
        _animations.push_back(qMove(targetAnimation));
        _animations.push_back(qMove(zoomOutAnimation));
        _animations.push_back(qMove(zoomInAnimation));
    }
}

void OsmAnd::MapAnimator_P::parabolicAnimateTargetWith(const PointD& velocity, const PointD& deceleration)
{
    const auto duration = qSqrt((velocity.x*velocity.x + velocity.y*velocity.y) / (deceleration.x*deceleration.x + deceleration.y*deceleration.y));
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    parabolicAnimateTargetBy(deltaValue, duration, MapAnimatorTimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::animateAzimuthBy(const float deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    std::shared_ptr<AbstractAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        deltaValue, duration, 0.0f, timingFunction,
        _azimuthGetter, _azimuthSetter));

    {
        QMutexLocker scopedLocker(&_animationsMutex);
        _animations.push_back(qMove(newAnimation));
    }
}

void OsmAnd::MapAnimator_P::animateAzimuthWith(const float velocity, const float deceleration)
{
    const auto duration = qAbs(velocity / deceleration);
    const auto deltaValue = 0.5f * velocity * duration;

    animateAzimuthBy(deltaValue, duration, MapAnimatorTimingFunction::EaseOutQuadratic);
}

void OsmAnd::MapAnimator_P::animateElevationAngleBy(const float deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    std::shared_ptr<AbstractAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        deltaValue, duration, 0.0f, timingFunction,
        _elevationAngleGetter, _elevationAngleSetter));

    {
        QMutexLocker scopedLocker(&_animationsMutex);
        _animations.push_back(qMove(newAnimation));
    }
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
    const auto halfDuration = duration / 2.0f;

    std::shared_ptr<AbstractAnimation> targetAnimation(new MapAnimator_P::Animation<PointI64>(
        deltaValue, duration, 0.0f, timingFunction,
        _targetGetter, _targetSetter));

    const std::shared_ptr<AnimationContext> sharedContext(new AnimationContext());
    std::shared_ptr<AbstractAnimation> zoomOutAnimation(new MapAnimator_P::Animation<float>(
        [this, deltaValue](AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
        {
            // Find out the final target
            const auto& currentTarget = _renderer->state.target31;
            const auto finalTarget = Utilities::normalizeCoordinates(PointI64(currentTarget) + deltaValue, _renderer->state.zoomBase);

            // THIS ACTUALLY HAS NO SENSE. FORMULA SHOULD GIVE 0 or LESS on SAME SCREEN
            //// If final target is in the visible area, don't use parabolic effect
            //if(_renderer->isVisible(finalTarget))
            //    return 0.0f;

            const float zoomShift = 3.0f;
            //TODO: Recalculate delta into tiles (length / size_of_tile_in_31)
            //TODO: each '3 screens' of tiles-delta gives -1 to zoom

            sharedContext->storageList.push_back(QVariant(zoomShift));
            return -zoomShift;
        },
        halfDuration, 0.0f, MapAnimatorTimingFunction::EaseOutQuadratic,
        _zoomGetter, _zoomSetter, sharedContext));
    std::shared_ptr<AbstractAnimation> zoomInAnimation(new MapAnimator_P::Animation<float>(
        [this](AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
        {
            // If shared context contains no data it means that parabolic effect was disabled
            if(sharedContext->storageList.isEmpty())
                return 0.0f;

            // Just restore the original zoom
            return sharedContext->storageList.first().toFloat();
        },
        halfDuration, halfDuration, MapAnimatorTimingFunction::EaseInQuadratic,
        _zoomGetter, _zoomSetter, sharedContext));

    std::shared_ptr<AbstractAnimation> zeroizeAzimuthAnimation;
    if(zeroizeAzimuth)
    {
        zeroizeAzimuthAnimation.reset(new MapAnimator_P::Animation<float>(
            [this](AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
            {
                return -azimuthGetter(context, sharedContext);
            },
            duration, 0.0f, timingFunction,
            _azimuthGetter, _azimuthSetter));
    }

    std::shared_ptr<AbstractAnimation> invZeroizeElevationAngleAnimation;
    if(invZeroizeElevationAngle)
    {
        invZeroizeElevationAngleAnimation.reset(new MapAnimator_P::Animation<float>(
            [this](AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext)
            {
                return 90.0f - elevationAngleGetter(context, sharedContext);
            },
            duration, 0.0f, timingFunction,
            _elevationAngleGetter, _elevationAngleSetter));
    }

    {
        QMutexLocker scopedLocker(&_animationsMutex);
        _animations.push_back(qMove(targetAnimation));
        _animations.push_back(qMove(zoomOutAnimation));
        _animations.push_back(qMove(zoomInAnimation));
        if(zeroizeAzimuth)
            _animations.push_back(qMove(zeroizeAzimuthAnimation));
        if(invZeroizeElevationAngle)
            _animations.push_back(qMove(invZeroizeElevationAngleAnimation));
    }
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
