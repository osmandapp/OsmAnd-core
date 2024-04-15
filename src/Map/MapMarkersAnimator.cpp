#include "MapMarkersAnimator.h"
#include "MapMarkersAnimator_P.h"

#include <OsmAndCore/QtExtensions.h>
#include <QtMath>
#include <QMutableListIterator>

#include <IMapRenderer.h>
#include <Logging.h>
#include <Utilities.h>

OsmAnd::MapMarkersAnimator::MapMarkersAnimator()
    : _p(new MapMarkersAnimator_P(this))
    , mapRenderer(_p->_renderer)
{
}

OsmAnd::MapMarkersAnimator::~MapMarkersAnimator()
{
}

void OsmAnd::MapMarkersAnimator::setMapRenderer(const std::shared_ptr<IMapRenderer>& mapRenderer)
{
    _p->setMapRenderer(mapRenderer);
}

bool OsmAnd::MapMarkersAnimator::isPaused() const
{
    return _p->isPaused();
}

void OsmAnd::MapMarkersAnimator::pause()
{
    _p->pause();
}

void OsmAnd::MapMarkersAnimator::resume()
{
    _p->resume();
}

bool OsmAnd::MapMarkersAnimator::cancelAnimation(const std::shared_ptr<const IAnimation>& animation)
{
    return _p->cancelAnimation(animation);
}

QList< std::shared_ptr<const OsmAnd::IAnimation> > OsmAnd::MapMarkersAnimator::getAnimations(const std::shared_ptr<MapMarker> mapMarker) const
{
    return _p->getAnimations(mapMarker);
}

QList< std::shared_ptr<OsmAnd::IAnimation> > OsmAnd::MapMarkersAnimator::getAnimations(const std::shared_ptr<MapMarker> mapMarker)
{
    return _p->getAnimations(mapMarker);
}

bool OsmAnd::MapMarkersAnimator::pauseAnimations(const std::shared_ptr<MapMarker> mapMarker)
{
    return _p->pauseAnimations(mapMarker);
}

bool OsmAnd::MapMarkersAnimator::resumeAnimations(const std::shared_ptr<MapMarker> mapMarker)
{
    return _p->resumeAnimations(mapMarker);
}

bool OsmAnd::MapMarkersAnimator::cancelAnimations(const std::shared_ptr<MapMarker> mapMarker)
{
    return _p->cancelAnimations(mapMarker);
}

bool OsmAnd::MapMarkersAnimator::cancelCurrentAnimation(const std::shared_ptr<MapMarker> mapMarker, const AnimatedValue animatedValue)
{
    return _p->cancelCurrentAnimation(mapMarker, animatedValue);
}

std::shared_ptr<const OsmAnd::IAnimation> OsmAnd::MapMarkersAnimator::getCurrentAnimation(const std::shared_ptr<MapMarker> mapMarker, const AnimatedValue animatedValue) const
{
    return _p->getCurrentAnimation(mapMarker, animatedValue);
}

std::shared_ptr<OsmAnd::IAnimation> OsmAnd::MapMarkersAnimator::getCurrentAnimation(const std::shared_ptr<MapMarker> mapMarker, const AnimatedValue animatedValue)
{
    return _p->getCurrentAnimation(mapMarker, animatedValue);
}

QList< std::shared_ptr<const OsmAnd::IAnimation> > OsmAnd::MapMarkersAnimator::getAllAnimations() const
{
    return _p->getAllAnimations();
}

QList< std::shared_ptr<OsmAnd::IAnimation> > OsmAnd::MapMarkersAnimator::getAllAnimations()
{
    return _p->getAllAnimations();
}

void OsmAnd::MapMarkersAnimator::cancelAllAnimations()
{
    _p->cancelAllAnimations();
}

bool OsmAnd::MapMarkersAnimator::update(const float timePassed)
{
    return _p->update(timePassed);
}

void OsmAnd::MapMarkersAnimator::animatePositionBy(
    const std::shared_ptr<MapMarker> mapMarker,
    const PointI64& deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    _p->animatePositionBy(mapMarker, deltaValue, duration, timingFunction);
}

void OsmAnd::MapMarkersAnimator::animatePositionTo(
    const std::shared_ptr<MapMarker> mapMarker,
    const PointI& value,
    const float duration,
    const TimingFunction timingFunction)
{
    _p->animatePositionTo(mapMarker, value, duration, timingFunction);
}

void OsmAnd::MapMarkersAnimator::animatePositionWith(
    const std::shared_ptr<MapMarker> mapMarker,
    const PointD& velocity,
    const PointD& deceleration)
{
    _p->animatePositionWith(mapMarker, velocity, deceleration);
}

void OsmAnd::MapMarkersAnimator::animateDirectionBy(
    const std::shared_ptr<MapMarker> mapMarker,
    const OnSurfaceIconKey iconKey,
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    _p->animateDirectionBy(mapMarker, iconKey, deltaValue, duration, timingFunction);
}

void OsmAnd::MapMarkersAnimator::animateDirectionTo(
    const std::shared_ptr<MapMarker> mapMarker,
    const OnSurfaceIconKey iconKey,
    const float value,
    const float duration,
    const TimingFunction timingFunction)
{
    _p->animateDirectionTo(mapMarker, iconKey, value, duration, timingFunction);
}

void OsmAnd::MapMarkersAnimator::animateDirectionWith(
    const std::shared_ptr<MapMarker> mapMarker,
    const OnSurfaceIconKey iconKey,
    const float velocity,
    const float deceleration)
{
    _p->animateDirectionWith(mapMarker, iconKey, velocity, deceleration);
}

void OsmAnd::MapMarkersAnimator::animateModel3DDirectionBy(
    const std::shared_ptr<MapMarker> mapMarker,
    const float deltaValue,
    const float duration,
    const TimingFunction timingFunction)
{
    _p->animateModel3DDirectionBy(mapMarker, deltaValue, duration, timingFunction);
}

void OsmAnd::MapMarkersAnimator::animateModel3DDirectionTo(
    const std::shared_ptr<MapMarker> mapMarker,
    const float value,
    const float duration,
    const TimingFunction timingFunction)
{
    _p->animateModel3DDirectionTo(mapMarker, value, duration, timingFunction);
}

void OsmAnd::MapMarkersAnimator::animateModel3DDirectionWith(
    const std::shared_ptr<MapMarker> mapMarker,
    const float velocity,
    const float deceleration)
{
    _p->animateModel3DDirectionWith(mapMarker, velocity, deceleration);
}