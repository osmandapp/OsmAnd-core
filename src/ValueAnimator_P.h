#ifndef _OSMAND_CORE_VALUE_ANIMATOR_P_H_
#define _OSMAND_CORE_VALUE_ANIMATOR_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QReadWriteLock>
#include <QHash>
#include <QAtomicInt>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "ValueAnimator.h"

namespace OsmAnd
{
    class ValueAnimator_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ValueAnimator_P);
    public:
        typedef ValueAnimator::TimingFunction TimingFunction;
        typedef ValueAnimator::AnimationContext AnimationContext;
        typedef ValueAnimator::IAnimation IAnimation;
        typedef ValueAnimator::AnimationPredicate AnimationPredicate;
        typedef ValueAnimator::Key Key;
        typedef ValueAnimator::UntypedSignatures UntypedSignatures;
        typedef ValueAnimator::IUntypedValueMathOperations IUntypedValueMathOperations;

    private:
        typedef QList< std::shared_ptr<IAnimation> > AnimationsCollection;

        QAtomicInt _isPaused;
        mutable QMutex _updateLock;
        mutable QReadWriteLock _animationsCollectionLock;
        QHash<Key, AnimationsCollection> _animationsByKey;
    protected:
        ValueAnimator_P(ValueAnimator* const owner);
    public:
        ~ValueAnimator_P();

        ImplementationInterface<ValueAnimator> owner;

        bool isPaused() const;
        void pause();
        void resume();

        bool cancelAnimation(const std::shared_ptr<const IAnimation>& animation);

        QList< std::shared_ptr<IAnimation> > getAnimations(const Key key, const AnimationPredicate predicate);
        QList< std::shared_ptr<const IAnimation> > getAnimations(const Key key, const AnimationPredicate predicate) const;
        int pauseAnimations(const Key key, const AnimationPredicate predicate);
        int resumeAnimations(const Key key, const AnimationPredicate predicate);
        int cancelAnimations(const Key key, const AnimationPredicate predicate);

        QList< std::shared_ptr<IAnimation> > getAnimations(const AnimationPredicate predicate);
        QList< std::shared_ptr<const IAnimation> > getAnimations(const AnimationPredicate predicate) const;
        int pauseAnimations(const AnimationPredicate predicate);
        int resumeAnimations(const AnimationPredicate predicate);
        int cancelAnimations(const AnimationPredicate predicate);

        void update(const float timePassed);

        std::shared_ptr<IAnimation> animateValueBy(
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
            const std::shared_ptr<AnimationContext>& sharedContext);

    friend class OsmAnd::ValueAnimator;
    };
}

#endif // !defined(_OSMAND_CORE_VALUE_ANIMATOR_P_H_)
