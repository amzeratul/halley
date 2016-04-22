#pragma once

#include "../tool/clitool.h"

namespace Halley
{
	class CodegenTool : public CommandLineTool
	{
	public:
		void run(std::vector<std::string> args) override;
	};
}
