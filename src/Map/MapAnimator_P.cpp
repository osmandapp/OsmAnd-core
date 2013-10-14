#include "MapAnimator_P.h"
#include "MapAnimator.h"

#include <cassert>

#include <QtMath>

OsmAnd::MapAnimator_P::MapAnimator_P( MapAnimator* const owner_ )
    : owner(owner_)
    , _isAnimationPaused(true)
    , _animationsMutex(QMutex::Recursive)
{
}

OsmAnd::MapAnimator_P::~MapAnimator_P()
{

}

float OsmAnd::MapAnimator_P::AbstractAnimation::linearTween( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial + nt*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeIn_Quadratic( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial + (nt*nt)*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeOut_Quadratic( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial - nt*(nt-2.0f)*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeIn_Cubic( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial + nt*nt*nt*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeOut_Cubic( const float t, const float initial, const float delta, const float duration )
{
    auto nt = t / duration;
    nt -= 1.0f;
    return initial + (nt*nt*nt + 1.0f)*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeIn_Quartic( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial + nt*nt*nt*nt*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeOut_Quartic( const float t, const float initial, const float delta, const float duration )
{
    auto nt = t / duration;
    nt -= 1.0f;
    return initial - (nt*nt*nt*nt - 1.0f)*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeIn_Quintic( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial + nt*nt*nt*nt*nt*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeOut_Quintic( const float t, const float initial, const float delta, const float duration )
{
    auto nt = t / duration;
    nt -= 1.0f;
    return initial + (nt*nt*nt*nt*nt + 1.0f)*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeIn_Sinusoidal( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial + delta - qCos(nt * M_PI_2)*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeOut_Sinusoidal( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial + qSin(nt * M_PI_2)*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeIn_Exponential( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial + qPow( 2.0f, 10.0f * (nt - 1.0f) )*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeOut_Exponential( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial + ( -qPow( 2.0f, -10.0f * nt ) + 1.0f )*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeIn_Circular( const float t, const float initial, const float delta, const float duration )
{
    const auto nt = t / duration;
    return initial - (qSqrt(1.0f - nt*nt) - 1.0f)*delta;
}

float OsmAnd::MapAnimator_P::AbstractAnimation::easeOut_Circular( const float t, const float initial, const float delta, const float duration )
{
    auto nt = t / duration;
    nt -= 1.0f;
    return initial + qSqrt(1.0f - nt*nt)*delta;
}
