#ifndef _OSMAND_CORE_STOPWATCH_H_
#define _OSMAND_CORE_STOPWATCH_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Logging.h>

namespace OsmAnd
{
    class OSMAND_CORE_API Stopwatch Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(Stopwatch)
    private:
    protected:
        std::chrono::high_resolution_clock::time_point _start;
    public:
        inline Stopwatch(const bool autostart = false)
        {
            if (autostart)
                start();
        }

        inline ~Stopwatch()
        {
        }

        inline void start()
        {
            _start = std::chrono::high_resolution_clock::now();
        }

        inline float elapsed() const
        {
            return std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - _start).count();
        }
    };
}

#endif // !defined(_OSMAND_CORE_STOPWATCH_H_) 