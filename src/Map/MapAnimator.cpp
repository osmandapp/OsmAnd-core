#include "MapAnimator.h"
#include "MapAnimator_P.h"

#include <OsmAndCore/QtExtensions.h>
#include <QtMath>
#include <QMutableListIterator>

#include <IMapRenderer.h>
#include <Logging.h>
#include <Utilities.h>

OsmAnd::MapAnimator::MapAnimator()
    : _d(new MapAnimator_P(this))
    , mapRenderer(_d->_renderer)
{
}

OsmAnd::MapAnimator::~MapAnimator()
{
}

void OsmAnd::MapAnimator::setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer)
{
    _d->setMapRenderer(mapRenderer);
}

bool OsmAnd::MapAnimator::isAnimationPaused() const
{
    return _d->isAnimationPaused();
}

bool OsmAnd::MapAnimator::isAnimationRunning() const
{
    return _d->isAnimationRunning();
}

void OsmAnd::MapAnimator::pauseAnimation()
{
    _d->pauseAnimation();
}

void OsmAnd::MapAnimator::resumeAnimation()
{
    _d->resumeAnimation();
}

void OsmAnd::MapAnimator::cancelAnimation()
{
    _d->cancelAnimation();
}

void OsmAnd::MapAnimator::update(const float timePassed)
{
    _d->update(timePassed);
}

void OsmAnd::MapAnimator::animateZoomBy(const float deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    _d->animateZoomBy(deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator::animateZoomWith(const float velocity, const float deceleration)
{
    _d->animateZoomWith(velocity, deceleration);
}

void OsmAnd::MapAnimator::animateTargetBy(const PointI& deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    _d->animateTargetBy(deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator::animateTargetBy(const PointI64& deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    _d->animateTargetBy(deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator::animateTargetWith(const PointD& velocity, const PointD& deceleration)
{
    _d->animateTargetWith(velocity, deceleration);
}

void OsmAnd::MapAnimator::parabolicAnimateTargetBy(const PointI& deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    _d->parabolicAnimateTargetBy(deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator::parabolicAnimateTargetBy(const PointI64& deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    _d->parabolicAnimateTargetBy(deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator::parabolicAnimateTargetWith(const PointD& velocity, const PointD& deceleration)
{
    _d->parabolicAnimateTargetWith(velocity, deceleration);
}

void OsmAnd::MapAnimator::animateAzimuthBy(const float deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    _d->animateAzimuthBy(deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator::animateAzimuthWith(const float velocity, const float deceleration)
{
    _d->animateAzimuthWith(velocity, deceleration);
}

void OsmAnd::MapAnimator::animateElevationAngleBy(const float deltaValue, const float duration, const MapAnimatorTimingFunction timingFunction)
{
    _d->animateAzimuthBy(deltaValue, duration, timingFunction);
}

void OsmAnd::MapAnimator::animateElevationAngleWith(const float velocity, const float deceleration)
{
    _d->animateElevationAngleWith(velocity, deceleration);
}

void OsmAnd::MapAnimator::animateMoveBy(
    const PointI& deltaValue, const float duration,
    const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
    const MapAnimatorTimingFunction timingFunction)
{
    _d->animateMoveBy(deltaValue, duration, zeroizeAzimuth, invZeroizeElevationAngle, timingFunction);
}

void OsmAnd::MapAnimator::animateMoveBy(
    const PointI64& deltaValue, const float duration,
    const bool zeroizeAzimuth, const bool invZeroizeElevationAngle,
    const MapAnimatorTimingFunction timingFunction)
{
    _d->animateMoveBy(deltaValue, duration, zeroizeAzimuth, invZeroizeElevationAngle, timingFunction);
}

void OsmAnd::MapAnimator::animateMoveWith(const PointD& velocity, const PointD& deceleration, const bool zeroizeAzimuth, const bool invZeroizeElevationAngle)
{
    _d->animateMoveWith(velocity, deceleration, zeroizeAzimuth, invZeroizeElevationAngle);
}
