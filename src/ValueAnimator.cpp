#include "ValueAnimator.h"
#include "ValueAnimator_P.h"

OsmAnd::ValueAnimator::ValueAnimator()
    : _p(new ValueAnimator_P(this))
{
}

OsmAnd::ValueAnimator::~ValueAnimator()
{
}

bool OsmAnd::ValueAnimator::isPaused() const
{
    return _p->isPaused();
}

void OsmAnd::ValueAnimator::pause()
{
    _p->pause();
}

void OsmAnd::ValueAnimator::resume()
{
    _p->resume();
}

bool OsmAnd::ValueAnimator::cancelAnimation(const std::shared_ptr<const IAnimation>& animation)
{
    return _p->cancelAnimation(animation);
}

QList< std::shared_ptr<OsmAnd::ValueAnimator::IAnimation> > OsmAnd::ValueAnimator::getAnimations(
    const Key key,
    const AnimationPredicate predicate /*= nullptr*/)
{
    return _p->getAnimations(key, predicate);
}

QList< std::shared_ptr<const OsmAnd::ValueAnimator::IAnimation> > OsmAnd::ValueAnimator::getAnimations(
    const Key key,
    const AnimationPredicate predicate /*= nullptr*/) const
{
    return _p->getAnimations(key, predicate);
}

int OsmAnd::ValueAnimator::pauseAnimations(const Key key, const AnimationPredicate predicate /*= nullptr*/)
{
    return _p->pauseAnimations(key, predicate);
}

int OsmAnd::ValueAnimator::resumeAnimations(const Key key, const AnimationPredicate predicate /*= nullptr*/)
{
    return _p->resumeAnimations(key, predicate);
}

int OsmAnd::ValueAnimator::cancelAnimations(const Key key, const AnimationPredicate predicate /*= nullptr*/)
{
    return _p->cancelAnimations(key, predicate);
}

QList< std::shared_ptr<const OsmAnd::ValueAnimator::IAnimation> > OsmAnd::ValueAnimator::getAnimations(
    const AnimationPredicate predicate /*= nullptr*/) const
{
    return _p->getAnimations(predicate);
}

QList< std::shared_ptr<OsmAnd::ValueAnimator::IAnimation> > OsmAnd::ValueAnimator::getAnimations(
    const AnimationPredicate predicate /*= nullptr*/)
{
    return _p->getAnimations(predicate);
}

int OsmAnd::ValueAnimator::pauseAnimations(const AnimationPredicate predicate /*= nullptr*/)
{
    return _p->pauseAnimations(predicate);
}

int OsmAnd::ValueAnimator::resumeAnimations(const AnimationPredicate predicate /*= nullptr*/)
{
    return _p->resumeAnimations(predicate);
}

int OsmAnd::ValueAnimator::cancelAnimations(const AnimationPredicate predicate /*= nullptr*/)
{
    return _p->cancelAnimations(predicate);
}

void OsmAnd::ValueAnimator::update(const float timePassed)
{
    _p->update(timePassed);
}

std::shared_ptr<OsmAnd::ValueAnimator::IAnimation> OsmAnd::ValueAnimator::animateValueTo(
    const float duration,
    const float delay,
    const TimingFunction timingFunction,
    const UntypedSignatures::Allocator valueAllocator,
    const UntypedSignatures::Deallocator valueDeallocator,
    const UntypedSignatures::Getter finalValueGetter,
    const UntypedSignatures::Getter valueGetter,
    const UntypedSignatures::Setter valueSetter,
    const std::shared_ptr<const IUntypedValueMathOperations>& mathOps,
    const Key key /*= nullptr*/,
    const std::shared_ptr<AnimationContext>& sharedContext /*= nullptr*/)
{
    const UntypedSignatures::Getter deltaValueGetter =
        [valueAllocator, valueDeallocator, finalValueGetter, valueGetter, mathOps]
        (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext, void* const outValue)
        {
            const auto finalValue = valueAllocator();
            finalValueGetter(context, sharedContext, finalValue);

            const auto currentValue = valueAllocator();
            valueGetter(context, sharedContext, currentValue);

            mathOps->sub(outValue, finalValue, currentValue);

            valueDeallocator(finalValue);
            valueDeallocator(currentValue);
        };

    return animateValueBy(
        duration,
        delay,
        timingFunction,
        valueAllocator,
        valueDeallocator,
        deltaValueGetter,
        valueGetter,
        valueSetter,
        mathOps,
        key,
        sharedContext);
}

std::shared_ptr<OsmAnd::ValueAnimator::IAnimation> OsmAnd::ValueAnimator::animateValueBy(
    const float duration,
    const float delay,
    const TimingFunction timingFunction,
    const UntypedSignatures::Allocator valueAllocator,
    const UntypedSignatures::Deallocator valueDeallocator,
    const UntypedSignatures::Getter deltaValueGetter,
    const UntypedSignatures::Getter valueGetter,
    const UntypedSignatures::Setter valueSetter,
    const std::shared_ptr<const IUntypedValueMathOperations>& mathOps,
    const Key key /*= nullptr*/,
    const std::shared_ptr<AnimationContext>& sharedContext /*= nullptr*/)
{
    return _p->animateValueBy(
        duration,
        delay,
        timingFunction,
        valueAllocator,
        valueDeallocator,
        deltaValueGetter,
        valueGetter,
        valueSetter,
        mathOps,
        key,
        sharedContext);
}

OsmAnd::ValueAnimator::IUntypedValueMathOperations::IUntypedValueMathOperations()
{
}

OsmAnd::ValueAnimator::IUntypedValueMathOperations::~IUntypedValueMathOperations()
{
}
