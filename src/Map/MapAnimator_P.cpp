#include "MapAnimator_P.h"
#include "MapAnimator.h"

#include "IMapRenderer.h"
#include "Utilities.h"

OsmAnd::MapAnimator_P::MapAnimator_P( MapAnimator* const owner_ )
    : owner(owner_)
    , _isAnimationPaused(true)
    , _animationsMutex(QMutex::Recursive)
    , _zoomGetter(std::bind(&MapAnimator_P::zoomGetter, this))
    , _zoomSetter(std::bind(&MapAnimator_P::zoomSetter, this, std::placeholders::_1))
    , _azimuthGetter(std::bind(&MapAnimator_P::azimuthGetter, this))
    , _azimuthSetter(std::bind(&MapAnimator_P::azimuthSetter, this, std::placeholders::_1))
    , _elevationAngleGetter(std::bind(&MapAnimator_P::elevationAngleGetter, this))
    , _elevationAngleSetter(std::bind(&MapAnimator_P::elevationAngleSetter, this, std::placeholders::_1))
    , _targetGetter(std::bind(&MapAnimator_P::targetGetter, this))
    , _targetSetter(std::bind(&MapAnimator_P::targetSetter, this, std::placeholders::_1))
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

void OsmAnd::MapAnimator_P::animateZoomBy(const float deltaValue, const float duration, const MapAnimatorEasingType easingIn, const MapAnimatorEasingType easingOut)
{
    std::shared_ptr<AbstractAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        deltaValue, duration, easingIn, easingOut,
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

    animateZoomBy(deltaValue, duration, MapAnimatorEasingType::None, MapAnimatorEasingType::Quadratic);
}

void OsmAnd::MapAnimator_P::animateTargetBy(const PointI& deltaValue, const float duration, const MapAnimatorEasingType easingIn, const MapAnimatorEasingType easingOut)
{
    animateTargetBy(PointI64(deltaValue), duration, easingIn, easingOut);
}

void OsmAnd::MapAnimator_P::animateTargetBy(const PointI64& deltaValue, const float duration, const MapAnimatorEasingType easingIn, const MapAnimatorEasingType easingOut)
{
    std::shared_ptr<AbstractAnimation> newAnimation(new MapAnimator_P::Animation<PointI64>(
        deltaValue, duration, easingIn, easingOut,
        _targetGetter, _targetSetter));

    {
        QMutexLocker scopedLocker(&_animationsMutex);
        _animations.push_back(qMove(newAnimation));
    }
}

void OsmAnd::MapAnimator_P::animateTargetWith(const PointD& velocity, const PointD& deceleration)
{
    const auto duration = qSqrt(velocity.x*velocity.x + velocity.y*velocity.y) / qSqrt(deceleration.x*deceleration.x + deceleration.y*deceleration.y);
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    animateTargetBy(deltaValue, duration, MapAnimatorEasingType::None, MapAnimatorEasingType::Quadratic);
}

void OsmAnd::MapAnimator_P::animateAzimuthBy(const float deltaValue, const float duration, const MapAnimatorEasingType easingIn, const MapAnimatorEasingType easingOut)
{
    std::shared_ptr<AbstractAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        deltaValue, duration, easingIn, easingOut,
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

    animateAzimuthBy(deltaValue, duration, MapAnimatorEasingType::None, MapAnimatorEasingType::Quadratic);
}

void OsmAnd::MapAnimator_P::animateElevationAngleBy(const float deltaValue, const float duration, const MapAnimatorEasingType easingIn, const MapAnimatorEasingType easingOut)
{
    std::shared_ptr<AbstractAnimation> newAnimation(new MapAnimator_P::Animation<float>(
        deltaValue, duration, easingIn, easingOut,
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

    animateElevationAngleBy(deltaValue, duration, MapAnimatorEasingType::None, MapAnimatorEasingType::Quadratic);
}

void OsmAnd::MapAnimator_P::animateMoveBy(
    const PointI& deltaValue, const float duration,
    const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
    const MapAnimatorEasingType easingIn, const MapAnimatorEasingType easingOut)
{
    animateMoveBy(PointI64(deltaValue), duration, zeroizeAzimuth, invZeroizeElevationAngle, easingIn, easingOut);
}

void OsmAnd::MapAnimator_P::animateMoveBy(
    const PointI64& deltaValue, const float duration,
    const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
    const MapAnimatorEasingType easingIn, const MapAnimatorEasingType easingOut)
{
    std::shared_ptr<AbstractAnimation> targetAnimation(new MapAnimator_P::Animation<PointI64>(deltaValue, duration, easingIn, easingOut,
        _targetGetter, _targetSetter));

    std::shared_ptr<AbstractAnimation> zeroizeAzimuthAnimation;
    if(zeroizeAzimuth)
    {
        zeroizeAzimuthAnimation.reset(new MapAnimator_P::Animation<float>(
            [this]()
            {
                return -azimuthGetter();
            },
            duration, easingIn, easingOut,
            _azimuthGetter, _azimuthSetter));
    }

    std::shared_ptr<AbstractAnimation> invZeroizeElevationAngleAnimation;
    if(invZeroizeElevationAngle)
    {
        invZeroizeElevationAngleAnimation.reset(new MapAnimator_P::Animation<float>(
            [this]()
            {
                return 90.0f - elevationAngleGetter();
            },
            duration, easingIn, easingOut,
            _elevationAngleGetter, _elevationAngleSetter));
    }

    {
        QMutexLocker scopedLocker(&_animationsMutex);
        _animations.push_back(qMove(targetAnimation));
        if(zeroizeAzimuth)
            _animations.push_back(qMove(zeroizeAzimuthAnimation));
        if(invZeroizeElevationAngle)
            _animations.push_back(qMove(invZeroizeElevationAngleAnimation));
    }
}

void OsmAnd::MapAnimator_P::animateMoveWith(const PointD& velocity, const PointD& deceleration, const bool zeroizeAzimuth, const bool invZeroizeElevationAngle)
{
    const auto duration = qSqrt(velocity.x*velocity.x + velocity.y*velocity.y) / qSqrt(deceleration.x*deceleration.x + deceleration.y*deceleration.y);
    const PointI64 deltaValue(
        0.5f * velocity.x * duration,
        0.5f * velocity.y * duration);

    animateMoveBy(deltaValue, duration, zeroizeAzimuth, invZeroizeElevationAngle, MapAnimatorEasingType::None, MapAnimatorEasingType::Quadratic);
}

float OsmAnd::MapAnimator_P::zoomGetter()
{
    return _renderer->state.requestedZoom;
}

void OsmAnd::MapAnimator_P::zoomSetter(const float newValue)
{
    _renderer->setZoom(newValue);
}

float OsmAnd::MapAnimator_P::azimuthGetter()
{
    return _renderer->state.azimuth;
}

void OsmAnd::MapAnimator_P::azimuthSetter(const float newValue)
{
    _renderer->setAzimuth(newValue);
}

float OsmAnd::MapAnimator_P::elevationAngleGetter()
{
    return _renderer->state.elevationAngle;
}

void OsmAnd::MapAnimator_P::elevationAngleSetter(const float newValue)
{
    _renderer->setElevationAngle(newValue);
}

OsmAnd::PointI64 OsmAnd::MapAnimator_P::targetGetter()
{
    return _renderer->state.target31;
}

void OsmAnd::MapAnimator_P::targetSetter(const PointI64 newValue)
{
    _renderer->setTarget(Utilities::normalizeCoordinates(newValue, ZoomLevel31));
}
