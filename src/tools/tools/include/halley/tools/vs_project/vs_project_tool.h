#pragma once

#include "halley/tools/cli_tool.h"

namespace Halley
{
	class VSProjectTool : public CommandLineTool
	{
	public:
		int run(Vector<std::string> args) override;

	private:
		int copyFiles(const Vector<std::string>& args);
	};
}
