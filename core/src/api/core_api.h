#pragma once
#include "../stage/stage_id.h"

namespace Halley
{
	class CoreAPI
	{
	public:
		virtual ~CoreAPI() {}
		virtual void quit() = 0;
		virtual void setStage(StageID stage) = 0;
	};
}
