#pragma once

#include "icode_generator.h"
#include <halley/data_structures/hash_map.h>
#include <gsl/gsl>
#include "halley/file/path.h"

namespace YAML
{
	class Node;
}

namespace Halley
{
	class ECSData;
	class ComponentSchema;
	class SystemSchema;
	class MessageSchema;
	class CustomTypeSchema;
	
	class Codegen
	{	
		struct Stats
		{
			int written = 0;
			int skipped = 0;
			std::vector<Path> files;
		};

	public:
		using ProgressReporter = std::function<bool(float, String)>;

		static void run(Path inDir, Path outDir);
		static std::vector<Path> generateCode(const ECSData& data, Path directory);

	private:
		static bool writeFile(Path path, const char* data, size_t dataSize, bool stub);
		static void writeFiles(Path directory, const CodeGenResult& files, Stats& stats);
	};
}