#include "ElapsedTimer.h"

OsmAnd::ElapsedTimer::ElapsedTimer() : isEnabled(true), isRunning(false), elapsed(high_resolution_clock::duration(0)) {
}

void OsmAnd::ElapsedTimer::Enable() {
	isEnabled = true;
	elapsed = high_resolution_clock::duration(0);
}

void OsmAnd::ElapsedTimer::Disable() {
	Pause();
	isEnabled = false;
	elapsed = high_resolution_clock::duration(0);
}

void OsmAnd::ElapsedTimer::Reset() {
	elapsed = high_resolution_clock::duration(0);
	isRunning = false;
}

void OsmAnd::ElapsedTimer::Start() {
	if (!isEnabled) return;
	if (!isRunning) startPoint = high_resolution_clock::now();
	isRunning = true;
}

void OsmAnd::ElapsedTimer::Pause() {
	if (!isRunning) return;
	elapsed = elapsed + (high_resolution_clock::now() - startPoint);
	isRunning = false;
}

const high_resolution_clock::duration& OsmAnd::ElapsedTimer::GetElapsed() {
	Pause();
	return elapsed;
}

uint64_t OsmAnd::ElapsedTimer::GetElapsedMicros() {
	Pause();
	return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

uint64_t OsmAnd::ElapsedTimer::GetElapsedMs() {
	Pause();
	return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}
