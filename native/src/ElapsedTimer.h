#ifndef _OSMAND_ELAPSED_TIMER_H_
#define _OSMAND_ELAPSED_TIMER_H_

#include <chrono>
using namespace std::chrono;

namespace OsmAnd
{
    class ElapsedTimer
    {
    private:
        high_resolution_clock::duration elapsed;
        high_resolution_clock::time_point startPoint;
        bool isEnabled;
        bool isRunning;

    public:
        ElapsedTimer();

        void Enable();
        void Disable();

        void Start();
        void Pause();

        const high_resolution_clock::duration& GetElapsed();

        uint64_t GetElapsedMs();
    };
}

#endif