#include "ValueAnimator_P.h"
#include "ValueAnimator.h"

#include "QtExtensions.h"
#include "QtCommon.h"

OsmAnd::ValueAnimator_P::ValueAnimator_P(ValueAnimator* const owner_)
    : _isPaused(1)
    , owner(owner_)
{
}

OsmAnd::ValueAnimator_P::~ValueAnimator_P()
{
}

bool OsmAnd::ValueAnimator_P::isPaused() const
{
    return (_isPaused.loadAcquire() != 0);
}

void OsmAnd::ValueAnimator_P::pause()
{
    _isPaused.storeRelease(1);
}

void OsmAnd::ValueAnimator_P::resume()
{
    _isPaused.storeRelease(0);
}

bool OsmAnd::ValueAnimator_P::cancelAnimation(const std::shared_ptr<const IAnimation>& animation)
{
    QWriteLocker scopedLocker(&_animationsCollectionLock);

    const auto itAnimations = _animationsByKey.find(animation->getKey());
    if (itAnimations == _animationsByKey.end())
        return false;
    auto& animations = *itAnimations;

    bool wasRemoved = false;
    {
        auto itOtherAnimation = mutableIteratorOf(animations);
        while (itOtherAnimation.hasNext())
        {
            const auto& otherAnimation = itOtherAnimation.next();

            if (otherAnimation != animation)
                continue;

            itOtherAnimation.remove();
            wasRemoved = true;
            break;
        }
    }

    if (animations.isEmpty())
        _animationsByKey.erase(itAnimations);

    return wasRemoved;
}

QList< std::shared_ptr<OsmAnd::ValueAnimator_P::IAnimation> > OsmAnd::ValueAnimator_P::getAnimations(
    const Key key,
    const AnimationPredicate predicate)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return QList< std::shared_ptr<IAnimation> >();

    if (!predicate)
        return copyAs< QList< std::shared_ptr<IAnimation> > >(*citAnimations);

    QList< std::shared_ptr<IAnimation> > result;
    for (const auto& animation : constOf(*citAnimations))
    {
        if (predicate && !predicate(animation))
            continue;
        
        result.push_back(animation);
    }

    return result;
}

QList< std::shared_ptr<const OsmAnd::ValueAnimator_P::IAnimation> > OsmAnd::ValueAnimator_P::getAnimations(
    const Key key,
    const AnimationPredicate predicate) const
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return QList< std::shared_ptr<const IAnimation> >();

    QList< std::shared_ptr<const IAnimation> > result;
    for (const auto& animation : constOf(*citAnimations))
    {
        if (predicate && !predicate(animation))
            continue;

        result.push_back(animation);
    }

    return result;
}

int OsmAnd::ValueAnimator_P::pauseAnimations(const Key key, const AnimationPredicate predicate)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return -1;
    const auto& animations = *citAnimations;

    int pausedCount = 0;
    for (const auto& animation : constOf(animations))
    {
        if (predicate && !predicate(animation))
            continue;

        if (animation->pause())
            pausedCount++;
    }

    return pausedCount;
}

int OsmAnd::ValueAnimator_P::resumeAnimations(const Key key, const AnimationPredicate predicate)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    const auto citAnimations = _animationsByKey.constFind(key);
    if (citAnimations == _animationsByKey.cend())
        return -1;
    const auto& animations = *citAnimations;

    int resumedCount = 0;
    for (const auto& animation : constOf(animations))
    {
        if (predicate && !predicate(animation))
            continue;

        if (animation->resume())
            resumedCount++;
    }

    return resumedCount;
}

int OsmAnd::ValueAnimator_P::cancelAnimations(const Key key, const AnimationPredicate predicate)
{
    return -1;
}

QList< std::shared_ptr<OsmAnd::ValueAnimator_P::IAnimation> > OsmAnd::ValueAnimator_P::getAnimations(
    const AnimationPredicate predicate)
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    QList< std::shared_ptr<IAnimation> > result;
    for (const auto& animations : constOf(_animationsByKey))
    {
        for (const auto& animation : constOf(animations))
        {
            if (predicate && !predicate(animation))
                continue;

            result.push_back(animation);
        }
    }

    return result;
}

QList< std::shared_ptr<const OsmAnd::ValueAnimator_P::IAnimation> > OsmAnd::ValueAnimator_P::getAnimations(
    const AnimationPredicate predicate) const
{
    QReadLocker scopedLocker(&_animationsCollectionLock);

    QList< std::shared_ptr<const IAnimation> > result;
    for (const auto& animations : constOf(_animationsByKey))
    {
        for (const auto& animation : constOf(animations))
        {
            if (predicate && !predicate(animation))
                continue;

            result.push_back(animation);
        }
    }

    return result;
}

int OsmAnd::ValueAnimator_P::pauseAnimations(const AnimationPredicate predicate)
{
    return -1;
}

int OsmAnd::ValueAnimator_P::resumeAnimations(const AnimationPredicate predicate)
{
    return -1;
}

int OsmAnd::ValueAnimator_P::cancelAnimations(const AnimationPredicate predicate)
{
    return -1;
}

void OsmAnd::ValueAnimator_P::update(const float timePassed)
{

}

std::shared_ptr<OsmAnd::ValueAnimator_P::IAnimation> OsmAnd::ValueAnimator_P::animateValueBy(
    const float duration,
    const float delay,
    const TimingFunction timingFunction,
    const UntypedSignatures::Allocator valueAllocator,
    const UntypedSignatures::Deallocator valueDeallocator,
    const UntypedSignatures::Getter deltaValueGetter,
    const UntypedSignatures::Getter valueGetter,
    const UntypedSignatures::Setter valueSetter,
    const std::shared_ptr<const IUntypedValueMathOperations>& mathOps,
    const Key key,
    const std::shared_ptr<AnimationContext>& sharedContext)
{
    return nullptr;
}
