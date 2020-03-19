#pragma once

#include "halley/tools/cli_tool.h"

namespace Halley
{
	class RunnerTool : public CommandLineTool
	{
	public:
		int runRaw(int argc, char* argv[]) override;
	};
}
