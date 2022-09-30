#ifndef _OSMAND_CORE_I_ANIMATION_H_
#define _OSMAND_CORE_I_ANIMATION_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/AnimatorTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IAnimation
    {
        Q_DISABLE_COPY_AND_MOVE(IAnimation);

    private:
    protected:
        IAnimation();
    public:
        virtual ~IAnimation();

        virtual Animator::Key getKey() const = 0;
        virtual Animator::AnimatedValue getAnimatedValue() const = 0;

        virtual float getTimePassed() const = 0;
        virtual float getDelay() const = 0;
        virtual float getDuration() const = 0;
        virtual Animator::TimingFunction getTimingFunction() const = 0;

        virtual bool obtainInitialValueAsFloat(float& outValue) const = 0;
        virtual bool obtainDeltaValueAsFloat(float& outValue) const = 0;
        virtual bool obtainCurrentValueAsFloat(float& outValue) const = 0;

        virtual bool obtainInitialValueAsPointI64(PointI64& outValue) const = 0;
        virtual bool obtainDeltaValueAsPointI64(PointI64& outValue) const = 0;
        virtual bool obtainCurrentValueAsPointI64(PointI64& outValue) const = 0;

        virtual void pause() = 0;
        virtual void resume() = 0;
        virtual bool isPaused() const = 0;

        virtual bool isPlaying() const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_ANIMATION_H_)
