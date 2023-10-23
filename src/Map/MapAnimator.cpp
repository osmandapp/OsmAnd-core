#include "MapAnimator.h"
#include "MapAnimator_P.h"

#include <OsmAndCore/QtExtensions.h>
#include <QtMath>
#include <QMutableListIterator>

#include <IMapRenderer.h>
#include <Logging.h>
#include <Utilities.h>

OsmAnd::MapAnimator::MapAnimator(
    const bool suspendSymbolsDuringAnimation_ /*= true*/)
    : _p(new MapAnimator_P(this))
    , suspendSymbolsDuringAnimation(suspendSymbolsDuringAnimation_)
    , mapRenderer(_p->_renderer)
{
}

OsmAnd::MapAnimator::~MapAnimator()
{
}

void OsmAnd::MapAnimator::setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer)
{
    _p->setMapRenderer(mapRenderer);
}

bool OsmAnd::MapAnimator::isPaused() const
{
    return _p->isPaused();
}

void OsmAnd::MapAnimator::pause()
{
    _p->pause();
}

void OsmAnd::MapAnimator::resume()
{
    _p->resume();
}

bool OsmAnd::MapAnimator::cancelAnimation(const std::shared_ptr<const IAnimation>& animation)
{
    return _p->cancelAnimation(animation);
}

QList< std::shared_ptr<const OsmAnd::IAnimation> > OsmAnd::MapAnimator::getAnimations(const Key key) const
{
    return _p->getAnimations(key);
}

QList< std::shared_ptr<OsmAnd::IAnimation> > OsmAnd::MapAnimator::getAnimations(const Key key)
{
    return _p->getAnimations(key);
}

bool OsmAnd::MapAnimator::pauseAnimations(const Key key)
{
    return _p->pauseAnimations(key);
}

bool OsmAnd::MapAnimator::resumeAnimations(const Key key)
{
    return _p->resumeAnimations(key);
}

bool OsmAnd::MapAnimator::cancelAnimations(const Key key)
{
    return _p->cancelAnimations(key);
}

bool OsmAnd::MapAnimator::cancelCurrentAnimation(const Key key, const AnimatedValue animatedValue)
{
    return _p->cancelCurrentAnimation(key, animatedValue);
}

std::shared_ptr<const OsmAnd::IAnimation> OsmAnd::MapAnimator::getCurrentAnimation(const Key key, const AnimatedValue animatedValue) const
{
    return _p->getCurrentAnimation(key, animatedValue);
}

std::shared_ptr<OsmAnd::IAnimation> OsmAnd::MapAnimator::getCurrentAnimation(const Key key, const AnimatedValue animatedValue)
{
    return _p->getCurrentAnimation(key, animatedValue);
}

QList< std::shared_ptr<const OsmAnd::IAnimation> > OsmAnd::MapAnimator::getAllAnimations() const
{
    return _p->getAllAnimations();
}

QList< std::shared_ptr<OsmAnd::IAnimation> > OsmAnd::MapAnimator::getAllAnimations()
{
    return _p->getAllAnimations();
}

void OsmAnd::MapAnimator::cancelAllAnimations()
{
    _p->cancelAllAnimations();
}

bool OsmAnd::MapAnimator::update(const float timePassed)
{
    return _p->update(timePassed);
}

void OsmAnd::MapAnimator::animateZoomBy(
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateZoomBy(deltaValue, duration, timingFunction, key);
}

void OsmAnd::MapAnimator::animateZoomTo(
    const float value,
    const float duration,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateZoomTo(value, duration, timingFunction, key);
}

void OsmAnd::MapAnimator::animateZoomWith(
    const float velocity,
    const float deceleration,
    const Key key /*= nullptr*/)
{
    _p->animateZoomWith(velocity, deceleration, key);
}

void OsmAnd::MapAnimator::animateZoomToAndPan(
    const float value,
    const PointI& panValue,
    const float duration,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateZoomToAndPan(value, panValue, duration, timingFunction, key);
}

void OsmAnd::MapAnimator::animateTargetBy(
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateTargetBy(deltaValue, duration, timingFunction, key);
}

void OsmAnd::MapAnimator::animateTargetTo(
    const PointI& value,
    const float duration,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateTargetTo(value, duration, timingFunction, key);
}

void OsmAnd::MapAnimator::animateTargetWith(
    const PointD& velocity,
    const PointD& deceleration,
    const Key key /*= nullptr*/)
{
    _p->animateTargetWith(velocity, deceleration, key);
}

void OsmAnd::MapAnimator::parabolicAnimateTargetBy(
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction targetTimingFunction,
    const TimingFunction zoomTimingFunction,
    const Key key /*= nullptr*/)
{
    _p->parabolicAnimateTargetBy(deltaValue, duration, targetTimingFunction, zoomTimingFunction, key);
}

void OsmAnd::MapAnimator::parabolicAnimateTargetTo(
    const PointI& value,
    const float duration,
    const TimingFunction targetTimingFunction,
    const TimingFunction zoomTimingFunction,
    const Key key /*= nullptr*/)
{
    _p->parabolicAnimateTargetTo(value, duration, targetTimingFunction, zoomTimingFunction, key);
}

void OsmAnd::MapAnimator::parabolicAnimateTargetWith(
    const PointD& velocity,
    const PointD& deceleration,
    const Key key /*= nullptr*/)
{
    _p->parabolicAnimateTargetWith(velocity, deceleration, key);
}

void OsmAnd::MapAnimator::animateFlatTargetWith(
    const PointD& velocity,
    const PointD& deceleration,
    const Key key /*= nullptr*/)
{
    _p->animateFlatTargetWith(velocity, deceleration, key);
}

void OsmAnd::MapAnimator::animateAzimuthBy(
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateAzimuthBy(deltaValue, duration, timingFunction, key);
}

void OsmAnd::MapAnimator::animateAzimuthTo(
    const float value,
    const float duration,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateAzimuthTo(value, duration, timingFunction, key);
}

void OsmAnd::MapAnimator::animateAzimuthWith(
    const float velocity,
    const float deceleration,
    const Key key /*= nullptr*/)
{
    _p->animateAzimuthWith(velocity, deceleration, key);
}

void OsmAnd::MapAnimator::animateElevationAngleBy(
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateElevationAngleBy(deltaValue, duration, timingFunction, key);
}

void OsmAnd::MapAnimator::animateElevationAngleTo(
    const float value,
    const float duration,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateElevationAngleTo(value, duration, timingFunction, key);
}

void OsmAnd::MapAnimator::animateElevationAngleWith(
    const float velocity,
    const float deceleration,
    const Key key /*= nullptr*/)
{
    _p->animateElevationAngleWith(velocity, deceleration, key);
}

void OsmAnd::MapAnimator::animateMoveBy(
    const PointI64& deltaValue,
    const float duration,
    const bool zeroizeAzimuth,
    const bool invZeroizeElevationAngle,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateMoveBy(deltaValue, duration, zeroizeAzimuth, invZeroizeElevationAngle, timingFunction, key);
}

void OsmAnd::MapAnimator::animateMoveTo(
    const PointI& value,
    const float duration,
    const bool zeroizeAzimuth,
    const bool invZeroizeElevationAngle,
    const TimingFunction timingFunction,
    const Key key /*= nullptr*/)
{
    _p->animateMoveTo(value, duration, zeroizeAzimuth, invZeroizeElevationAngle, timingFunction, key);
}

void OsmAnd::MapAnimator::animateMoveWith(
    const PointD& velocity,
    const PointD& deceleration,
    const bool zeroizeAzimuth,
    const bool invZeroizeElevationAngle,
    const Key key /*= nullptr*/)
{
    _p->animateMoveWith(velocity, deceleration, zeroizeAzimuth, invZeroizeElevationAngle, key);
}

void OsmAnd::MapAnimator::animateLocationFixationOnScreen(
    const PointI& location31,
    const PointI& screenPoint,
    const bool azimuthChangeAllowed,
    const float duration,
    const Key key /*= nullptr*/)
{
    _p->animateLocationFixationOnScreen(location31, screenPoint, azimuthChangeAllowed, duration, key);
}