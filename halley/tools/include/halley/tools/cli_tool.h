#pragma once
#include <string>
#include <halley/data_structures/vector.h>
#include <memory>

namespace Halley
{
	class CommandLineTool
	{
	public:
		virtual ~CommandLineTool() {}

		static Vector<std::string> getToolNames();
		static std::unique_ptr<CommandLineTool> getTool(std::string name);

		int runRaw(int argc, char** argv);
		virtual int run(Vector<std::string> args) = 0;
	};
}
