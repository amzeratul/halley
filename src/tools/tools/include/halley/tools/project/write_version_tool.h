#pragma once

#include "halley/tools/cli_tool.h"

namespace Halley
{
	class WriteVersionTool : public CommandLineTool
	{
	public:
		int run(Vector<std::string> args) override;
	};

	class WriteCodeVersionTool : public CommandLineTool
	{
	public:
		int run(Vector<std::string> args) override;
	};
}
