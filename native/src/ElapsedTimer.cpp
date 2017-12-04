#include "ElapsedTimer.h"

OsmAnd::ElapsedTimer::ElapsedTimer()
    : isEnabled(true)
    , isRunning(false)
{
}

void OsmAnd::ElapsedTimer::Enable()
{
	isEnabled = true;
}

void OsmAnd::ElapsedTimer::Disable()
{
	Pause();
	isEnabled = false;
}

void OsmAnd::ElapsedTimer::Start()
{
	if (!isEnabled)
		return;
	if (!isRunning)
        startPoint = high_resolution_clock::now();
	isRunning = true;
}

void OsmAnd::ElapsedTimer::Pause()
{
	if (!isRunning)
		return;
    elapsed = (high_resolution_clock::now() - startPoint);
	isRunning = false;
}

const high_resolution_clock::duration& OsmAnd::ElapsedTimer::GetElapsed()
{
	Pause();
	return elapsed;
}

uint64_t OsmAnd::ElapsedTimer::GetElapsedMs()
{
    Pause();
    return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

