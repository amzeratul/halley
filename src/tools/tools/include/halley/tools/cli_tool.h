#pragma once
#include <string>
#include <halley/data_structures/vector.h>
#include <halley/text/halleystring.h>
#include <memory>
#include <functional>
#include <map>
#include "halley/core/game/environment.h"

namespace Halley
{
	class HalleyStatics;

	class CommandLineTool
	{
	public:
		virtual ~CommandLineTool();

		int runRaw(int argc, char** argv);
		virtual int run(Vector<std::string> args) = 0;

	protected:
		std::unique_ptr<HalleyStatics> statics;
		String platform;
		Environment env;
	};

	class CommandLineTools
	{
	public:
		CommandLineTools();

		Vector<std::string> getToolNames();
		std::unique_ptr<CommandLineTool> getTool(std::string name);

	private:
		using ToolFactory = std::function<std::unique_ptr<CommandLineTool>()>;
		std::map<std::string, ToolFactory> factories;
	};
}
