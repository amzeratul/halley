#include "halley/game/frame_data.h"

#include "halley/utils/algorithm.h"

using namespace Halley;

void BaseFrameData::baseStartFrame(bool multithreaded, BaseFrameData* previous, Time deltaTime)
{
	if (multithreaded && previous) {
		debugTexts = previous->debugTexts;
		for (auto dt: debugTexts) {
			dt.second.time -= deltaTime;
		}
		std_ex::erase_if_value(debugTexts, [&](const DebugText& dt)
		{
			return dt.time <= 0;
		});
	}

	if (previous) {
		frameIdx = previous->frameIdx;
	}

	debugLines.clear();
	debugPoints.clear();
	debugPolygons.clear();
	debugEllipses.clear();
	scriptStates.clear();
	debugWorldTexts.clear();
	uiRootData.clear();
}
