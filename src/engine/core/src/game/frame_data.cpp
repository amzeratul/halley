#include "halley/game/frame_data.h"

#include "halley/graphics/sprite/sprite_painter.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

BaseFrameData::BaseFrameData()
{
	painters.push_back(std::make_unique<SpritePainter>());
}

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

		for (size_t i = 0; i < painters.size(); ++i) {
			painters[i]->copyPrevious(*previous->painters.at(i));
		}
	}

	if (previous) {
		frameIdx = previous->frameIdx;
	}

	for (auto& painter: painters) {
		painter->startFrame(multithreaded);
	}
}

void BaseFrameData::baseEndFrame()
{
	debugLines.clear();
	debugPoints.clear();
	debugPolygons.clear();
	debugEllipses.clear();
	scriptStates.clear();
	debugWorldTexts.clear();
	uiRootData.clear();

	cameras.clear();
	renderGraphCommands.clear();
	zoomLevel = 1;
}
