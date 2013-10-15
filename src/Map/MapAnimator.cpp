#include "MapAnimator.h"
#include "MapAnimator_P.h"

#include <QtMath>
#include <QMutableListIterator>

#include <IMapRenderer.h>
#include <Utilities.h>

OsmAnd::MapAnimator::MapAnimator()
    : _d(new MapAnimator_P(this))
    , mapRenderer(_d->_renderer)
{
}

OsmAnd::MapAnimator::~MapAnimator()
{
}

void OsmAnd::MapAnimator::setMapRenderer( const std::shared_ptr<IMapRenderer>& mapRenderer )
{
    QMutexLocker scopedLocker(&_d->_animationsMutex);

    cancelAnimation();
    _d->_renderer = mapRenderer;
}

bool OsmAnd::MapAnimator::isAnimationPaused() const
{
    return _d->_isAnimationPaused;
}

bool OsmAnd::MapAnimator::isAnimationRunning() const
{
    return !_d->_isAnimationPaused && !_d->_animations.isEmpty();
}

void OsmAnd::MapAnimator::pauseAnimation()
{
    _d->_isAnimationPaused = true;
}

void OsmAnd::MapAnimator::resumeAnimation()
{
    _d->_isAnimationPaused = false;
}

void OsmAnd::MapAnimator::cancelAnimation()
{
    QMutexLocker scopedLocker(&_d->_animationsMutex);

    _d->_isAnimationPaused = true;
    _d->_animations.clear();
}

void OsmAnd::MapAnimator::update( const float timePassed )
{
    // Do nothing if animation is paused
    if(_d->_isAnimationPaused)
        return;

    // Apply all animations
    QMutexLocker scopedLocker(&_d->_animationsMutex);
    QMutableListIterator< std::shared_ptr<MapAnimator_P::AbstractAnimation> > itAnimation(_d->_animations);
    while(itAnimation.hasNext())
    {
        const auto& animation = itAnimation.next();

        if(animation->process(timePassed))
            itAnimation.remove();
    }
}

void OsmAnd::MapAnimator::animateZoomBy( const float deltaValue, const float duration, MapAnimatorEasingType easingIn /*= MapAnimatorEasingType::Quadratic*/, MapAnimatorEasingType easingOut /*= MapAnimatorEasingType::Quadratic*/ )
{
    std::shared_ptr<MapAnimator_P::AbstractAnimation> newAnimation(new MapAnimator_P::Animation<float>(deltaValue, duration, easingIn, easingOut,
        [this]()
        {
            return _d->_renderer->state.requestedZoom;
        },
        [this](const float& newValue)
        {
            _d->_renderer->setZoom(newValue);
        }));

    {
        QMutexLocker scopedLocker(&_d->_animationsMutex);
        _d->_animations.push_back(newAnimation);
    }
}

void OsmAnd::MapAnimator::animateZoomWith( const float velocity, const float deceleration )
{
    const auto duration = velocity / deceleration;
    const auto deltaValue = (3.0f / 2.0f)*(velocity*velocity/deceleration);

    animateZoomBy(deltaValue, duration, MapAnimatorEasingType::None, MapAnimatorEasingType::Quadratic);
}

void OsmAnd::MapAnimator::animateTargetBy( const PointI& deltaValue, const float duration, MapAnimatorEasingType easingIn /*= MapAnimatorEasingType::Quadratic*/, MapAnimatorEasingType easingOut /*= MapAnimatorEasingType::Quadratic*/ )
{
    std::shared_ptr<MapAnimator_P::AbstractAnimation> newAnimation(new MapAnimator_P::Animation<PointI>(deltaValue, duration, easingIn, easingOut,
        [this]()
        {
            return _d->_renderer->state.target31;
        },
        [this](const PointI& newValue)
        {
            _d->_renderer->setTarget(newValue);
        }));

    {
        QMutexLocker scopedLocker(&_d->_animationsMutex);
        _d->_animations.push_back(newAnimation);
    }
}

void OsmAnd::MapAnimator::animateTargetBy( const PointI64& deltaValue, const float duration, MapAnimatorEasingType easingIn /*= MapAnimatorEasingType::Quadratic*/, MapAnimatorEasingType easingOut /*= MapAnimatorEasingType::Quadratic*/ )
{
    std::shared_ptr<MapAnimator_P::AbstractAnimation> newAnimation(new MapAnimator_P::Animation<PointI64>(deltaValue, duration, easingIn, easingOut,
        [this]()
        {
            const auto value = _d->_renderer->state.target31;
            return PointI64(value.x, value.y);
        },
        [this](const PointI64& newValue)
        {
            _d->_renderer->setTarget(Utilities::normalizeCoordinates(newValue, ZoomLevel31));
        }));

    {
        QMutexLocker scopedLocker(&_d->_animationsMutex);
        _d->_animations.push_back(newAnimation);
    }
}

void OsmAnd::MapAnimator::animateTargetWith( const PointD& velocity, const PointD& deceleration )
{
    const auto duration = qSqrt(velocity.x*velocity.x + velocity.y*velocity.y) / qSqrt(deceleration.x*deceleration.x + deceleration.y*deceleration.y);
    const PointI64 deltaValue(
        (3.0 / 2.0)*(velocity.x*velocity.x/deceleration.x),
        (3.0 / 2.0)*(velocity.y*velocity.y/deceleration.y));

    animateTargetBy(deltaValue, duration, MapAnimatorEasingType::None, MapAnimatorEasingType::Quadratic);
}

void OsmAnd::MapAnimator::animateAzimuthBy( const float deltaValue, const float duration, MapAnimatorEasingType easingIn /*= MapAnimatorEasingType::Quadratic*/, MapAnimatorEasingType easingOut /*= MapAnimatorEasingType::Quadratic*/ )
{
    std::shared_ptr<MapAnimator_P::AbstractAnimation> newAnimation(new MapAnimator_P::Animation<float>(deltaValue, duration, easingIn, easingOut,
        [this]()
        {
            return _d->_renderer->state.azimuth;
        },
        [this](const float& newValue)
        {
            _d->_renderer->setAzimuth(newValue);
        }));

    {
        QMutexLocker scopedLocker(&_d->_animationsMutex);
        _d->_animations.push_back(newAnimation);
    }
}

void OsmAnd::MapAnimator::animateAzimuthWith( const float velocity, const float deceleration )
{
    const auto duration = velocity / deceleration;
    const auto deltaValue = (3.0f / 2.0f)*(velocity*velocity/deceleration);

    animateAzimuthBy(deltaValue, duration, MapAnimatorEasingType::None, MapAnimatorEasingType::Quadratic);
}

void OsmAnd::MapAnimator::animateElevationAngleBy( const float deltaValue, const float duration, MapAnimatorEasingType easingIn /*= MapAnimatorEasingType::Quadratic*/, MapAnimatorEasingType easingOut /*= MapAnimatorEasingType::Quadratic*/ )
{
    std::shared_ptr<MapAnimator_P::AbstractAnimation> newAnimation(new MapAnimator_P::Animation<float>(deltaValue, duration, easingIn, easingOut,
        [this]()
        {
            return _d->_renderer->state.elevationAngle;
        },
        [this](const float& newValue)
        {
            _d->_renderer->setElevationAngle(newValue);
        }));

    {
        QMutexLocker scopedLocker(&_d->_animationsMutex);
        _d->_animations.push_back(newAnimation);
    }
}

void OsmAnd::MapAnimator::animateElevationAngleWith( const float velocity, const float deceleration )
{
    const auto duration = velocity / deceleration;
    const auto deltaValue = (3.0f / 2.0f)*(velocity*velocity/deceleration);

    animateElevationAngleBy(deltaValue, duration, MapAnimatorEasingType::None, MapAnimatorEasingType::Quadratic);
}
