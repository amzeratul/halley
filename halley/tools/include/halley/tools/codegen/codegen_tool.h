#pragma once

#include "halley/tools/cli_tool.h"

namespace Halley
{
	class CodegenTool : public CommandLineTool
	{
	public:
		int run(Vector<std::string> args) override;
	};
}
