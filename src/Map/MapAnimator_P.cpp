#include "MapAnimator_P.h"
#include "MapAnimator.h"

OsmAnd::MapAnimator_P::MapAnimator_P( MapAnimator* const owner_ )
    : owner(owner_)
    , _isAnimationPaused(true)
    , _animationsMutex(QMutex::Recursive)
{
}

OsmAnd::MapAnimator_P::~MapAnimator_P()
{

}
