#ifndef _OSMAND_CORE_VALUE_ANIMATOR_H_
#define _OSMAND_CORE_VALUE_ANIMATOR_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QVariant>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    class ValueAnimator_P;
    class OSMAND_CORE_API ValueAnimator
    {
        Q_DISABLE_COPY_AND_MOVE(ValueAnimator)
    public:
        typedef const void* Key;

        enum class TimingFunction
        {
            Invalid = -1,

            Linear = 0,

#define DECLARE_VALUE_ANIMATOR_EASE_TIMING_FUNCTION(name)                                                                       \
    EaseIn##name,                                                                                                               \
    EaseOut##name,                                                                                                              \
    EaseInOut##name,                                                                                                            \
    EaseOutIn##name
            DECLARE_VALUE_ANIMATOR_EASE_TIMING_FUNCTION(Quadratic),
            DECLARE_VALUE_ANIMATOR_EASE_TIMING_FUNCTION(Cubic),
            DECLARE_VALUE_ANIMATOR_EASE_TIMING_FUNCTION(Quartic),
            DECLARE_VALUE_ANIMATOR_EASE_TIMING_FUNCTION(Quintic),
            DECLARE_VALUE_ANIMATOR_EASE_TIMING_FUNCTION(Sinusoidal),
            DECLARE_VALUE_ANIMATOR_EASE_TIMING_FUNCTION(Exponential),
            DECLARE_VALUE_ANIMATOR_EASE_TIMING_FUNCTION(Circular),
#undef DECLARE_VALUE_ANIMATOR_EASE_TIMING_FUNCTION
        };

        struct AnimationContext
        {
            AnimationContext();
            virtual ~AnimationContext();

            QVariantList storageList;
            QVariantHash storageHash;
        };

        struct UntypedSignatures Q_DECL_FINAL
        {
            typedef std::function < void*() > Allocator;
            typedef std::function < void(void* const value) > Deallocator;

            typedef std::function < void(
                AnimationContext& context,
                const std::shared_ptr<AnimationContext>& sharedContext,
                void* const outValue) > Getter;

            typedef std::function < void(
                AnimationContext& context,
                const std::shared_ptr<AnimationContext>& sharedContext,
                const void* const value) > Setter;

            typedef std::function < void(
                void* const outValue,
                const void* const leftValue,
                const void* const rightValue) > BinaryOperation;
        };

        struct OSMAND_CORE_API IUntypedValueMathOperations
        {
            IUntypedValueMathOperations();
            virtual ~IUntypedValueMathOperations();

            virtual void sub(void* const outValue, const void* const leftValue, const void* const rightValue) const = 0;
            virtual void add(void* const outValue, const void* const leftValue, const void* const rightValue) const = 0;
        };

        template<typename VALUE_TYPE>
        struct TypedValueMathOperations Q_DECL_FINAL : IUntypedValueMathOperations
        {
            TypedValueMathOperations()
            {
            }

            virtual ~TypedValueMathOperations()
            {
            }

            virtual void sub(void* const outValue, const void* const leftValue, const void* const rightValue) const
            {
                *reinterpret_cast<VALUE_TYPE*>(outValue) =
                    *reinterpret_cast<const VALUE_TYPE*>(leftValue)-*reinterpret_cast<const VALUE_TYPE*>(rightValue);
            }

            virtual void add(void* const outValue, const void* const leftValue, const void* const rightValue) const
            {
                *reinterpret_cast<VALUE_TYPE*>(outValue) =
                    *reinterpret_cast<const VALUE_TYPE*>(leftValue)+*reinterpret_cast<const VALUE_TYPE*>(rightValue);
            }
        };

        class OSMAND_CORE_API IAnimation
        {
            Q_DISABLE_COPY_AND_MOVE(IAnimation)

        private:
        protected:
            IAnimation();
        public:
            virtual ~IAnimation();

            virtual Key getKey() const = 0;
            virtual AnimationContext& getContext() = 0;
            virtual const AnimationContext& getContext() const = 0;
            virtual std::shared_ptr<AnimationContext> getSharedContext() = 0;
            virtual std::shared_ptr<const AnimationContext> getSharedContext() const = 0;
            
            virtual float getTimePassed() const = 0;
            virtual float getDelay() const = 0;
            virtual float getDuration() const = 0;
            virtual TimingFunction getTimingFunction() const = 0;

            virtual size_t getValueSize() const = 0;
            virtual bool obtainInitialValue(void* const outValue) const = 0;
            virtual bool obtainDeltaValue(void* const outValue) const = 0;
            virtual bool obtainCurrentValue(void* const outValue) const = 0;

            virtual bool pause() = 0;
            virtual bool resume() = 0;
            virtual bool isPaused() const = 0;

            virtual bool isPlaying() const = 0;
        };

        typedef std::function<bool(const std::shared_ptr<const IAnimation>& animation)> AnimationPredicate;

        template<typename VALUE_TYPE>
        struct TypedSignatures Q_DECL_FINAL
        {
            typedef std::function < void(
                AnimationContext& context,
                const std::shared_ptr<AnimationContext>& sharedContext,
                VALUE_TYPE& outValue) > Getter;

            typedef std::function < void(
                AnimationContext& context,
                const std::shared_ptr<AnimationContext>& sharedContext,
                const VALUE_TYPE& value) > Setter;
        };

        template<typename VALUE_TYPE>
        struct ConstantValueGetter Q_DECL_FINAL
        {
            ConstantValueGetter(const VALUE_TYPE value_)
                : value(value_)
            {
            }

            const VALUE_TYPE value;

            void operator()(
                AnimationContext& context,
                const std::shared_ptr<AnimationContext>& sharedContext,
                VALUE_TYPE& outValue) const
            {
                outValue = value;
            }

            operator typename TypedSignatures<VALUE_TYPE>::Getter() const
            {
                return std::bind(
                    &ConstantValueGetter::operator(),
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3);
            }
        };

        template<typename VALUE_TYPE>
        struct SimpleValueGetter Q_DECL_FINAL
        {
            typedef std::function<void(VALUE_TYPE& outValue)> ValueGetter;

            SimpleValueGetter(const ValueGetter valueGetter_)
                : valueGetter(valueGetter_)
            {
            }

            const ValueGetter valueGetter;

            void operator()(
                AnimationContext& context,
                const std::shared_ptr<AnimationContext>& sharedContext,
                VALUE_TYPE& outValue) const
            {
                valueGetter(outValue);
            }

            operator typename TypedSignatures<VALUE_TYPE>::Getter() const
            {
                return std::bind(
                    &SimpleValueGetter::operator(),
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3);
            }
        };
    private:
        PrivateImplementation<ValueAnimator_P> _p;
    protected:
    public:
        ValueAnimator();
        virtual ~ValueAnimator();

        bool isPaused() const;
        void pause();
        void resume();

        bool cancelAnimation(const std::shared_ptr<const IAnimation>& animation);

        QList< std::shared_ptr<IAnimation> > getAnimations(const Key key, const AnimationPredicate predicate = nullptr);
        QList< std::shared_ptr<const IAnimation> > getAnimations(
            const Key key,
            const AnimationPredicate predicate = nullptr) const;
        int pauseAnimations(const Key key, const AnimationPredicate predicate = nullptr);
        int resumeAnimations(const Key key, const AnimationPredicate predicate = nullptr);
        int cancelAnimations(const Key key, const AnimationPredicate predicate = nullptr);

        QList< std::shared_ptr<IAnimation> > getAnimations(const AnimationPredicate predicate = nullptr);
        QList< std::shared_ptr<const IAnimation> > getAnimations(const AnimationPredicate predicate = nullptr) const;
        int pauseAnimations(const AnimationPredicate predicate = nullptr);
        int resumeAnimations(const AnimationPredicate predicate = nullptr);
        int cancelAnimations(const AnimationPredicate predicate = nullptr);

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
            const Key key = nullptr,
            const std::shared_ptr<AnimationContext>& sharedContext = nullptr);

        std::shared_ptr<IAnimation> animateValueTo(
            const float duration,
            const float delay,
            const TimingFunction timingFunction,
            const UntypedSignatures::Allocator valueAllocator,
            const UntypedSignatures::Deallocator valueDeallocator,
            const UntypedSignatures::Getter finalValueGetter,
            const UntypedSignatures::Getter valueGetter,
            const UntypedSignatures::Setter valueSetter,
            const std::shared_ptr<const IUntypedValueMathOperations>& mathOps,
            const Key key = nullptr,
            const std::shared_ptr<AnimationContext>& sharedContext = nullptr);

        template<typename VALUE_TYPE>
        std::shared_ptr<IAnimation> animateValueTo(
            const float duration,
            const float delay,
            const TimingFunction timingFunction,
            const VALUE_TYPE finalValue,
            const typename TypedSignatures<VALUE_TYPE>::Getter valueGetter,
            const typename TypedSignatures<VALUE_TYPE>::Setter valueSetter,
            const Key key = nullptr,
            const std::shared_ptr<AnimationContext>& sharedContext = nullptr)
        {
            return animateValueTo<VALUE_TYPE>(
                duration,
                delay,
                timingFunction,
                ConstantValueGetter<VALUE_TYPE>(finalValue),
                valueGetter,
                valueSetter,
                key,
                sharedContext);
        }

        template<typename VALUE_TYPE>
        std::shared_ptr<IAnimation> animateValueTo(
            const float duration,
            const float delay,
            const TimingFunction timingFunction,
            const typename TypedSignatures<VALUE_TYPE>::Getter finalValueGetter,
            const typename TypedSignatures<VALUE_TYPE>::Getter valueGetter,
            const typename TypedSignatures<VALUE_TYPE>::Setter valueSetter,
            const Key key = nullptr,
            const std::shared_ptr<AnimationContext>& sharedContext = nullptr)
        {
            const typename TypedSignatures<VALUE_TYPE>::Getter deltaValueGetter =
                [finalValueGetter, valueGetter]
                (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext, VALUE_TYPE& outValue)
                {
                    VALUE_TYPE finalValue;
                    finalValueGetter(context, sharedContext, finalValue);

                    VALUE_TYPE currentValue;
                    valueGetter(context, sharedContext, currentValue);

                    outValue = finalValue - currentValue;
                };

            return animateValueBy<VALUE_TYPE>(
                duration,
                delay,
                timingFunction,
                deltaValueGetter,
                valueGetter,
                valueSetter,
                key,
                sharedContext);
        }
        
        template<typename VALUE_TYPE>
        std::shared_ptr<IAnimation> animateValueBy(
            const float duration,
            const float delay,
            const TimingFunction timingFunction,
            const VALUE_TYPE deltaValue,
            const typename TypedSignatures<VALUE_TYPE>::Getter valueGetter,
            const typename TypedSignatures<VALUE_TYPE>::Setter valueSetter,
            const Key key = nullptr,
            const std::shared_ptr<AnimationContext>& sharedContext = nullptr)
        {
            return animateValueBy<VALUE_TYPE>(
                duration,
                delay,
                timingFunction,
                ConstantValueGetter<VALUE_TYPE>(deltaValue),
                valueGetter,
                valueSetter,
                key,
                sharedContext);
        }

        template<typename VALUE_TYPE>
        std::shared_ptr<IAnimation> animateValueBy(
            const float duration,
            const float delay,
            const TimingFunction timingFunction,
            const typename TypedSignatures<VALUE_TYPE>::Getter deltaValueGetter,
            const typename TypedSignatures<VALUE_TYPE>::Getter valueGetter,
            const typename TypedSignatures<VALUE_TYPE>::Setter valueSetter,
            const Key key = nullptr,
            const std::shared_ptr<AnimationContext>& sharedContext = nullptr)
        {
            const UntypedSignatures::Allocator valueAllocator =
                []
                () -> void*
                {
                    return reinterpret_cast<void*>(new VALUE_TYPE());
                };
            const UntypedSignatures::Deallocator valueDeallocator =
                []
                (void* const value)
                {
                    delete reinterpret_cast<VALUE_TYPE*>(value);
                };
            const UntypedSignatures::Getter untypedDeltaValueGetter =
                [deltaValueGetter]
                (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext, void* const outValue)
                {
                    deltaValueGetter(context, sharedContext, *reinterpret_cast<VALUE_TYPE*>(outValue));
                };
            const UntypedSignatures::Getter untypedValueGetter =
                [valueGetter]
                (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext, void* const outValue)
                {
                    valueGetter(context, sharedContext, *reinterpret_cast<VALUE_TYPE*>(outValue));
                };
            const UntypedSignatures::Setter untypedValueSetter =
                [valueSetter]
                (AnimationContext& context, const std::shared_ptr<AnimationContext>& sharedContext, const void* const value)
                {
                    valueSetter(context, sharedContext, *reinterpret_cast<const VALUE_TYPE*>(value));
                };

            return animateValueBy(
                duration,
                delay,
                timingFunction,
                valueAllocator,
                valueDeallocator,
                untypedDeltaValueGetter,
                untypedValueGetter,
                untypedValueSetter,
                std::shared_ptr<const IUntypedValueMathOperations>(new TypedValueMathOperations<VALUE_TYPE>()),
                key,
                sharedContext);
        }
    };
}

#endif // !defined(_OSMAND_CORE_VALUE_ANIMATOR_H_)
