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
			Vector<Path> files;
		};

	public:
		constexpr static int currentCodegenVersion = 104;
		
		using ProgressReporter = std::function<bool(float, String)>;

		static void run(Path inDir, Path outDir);
		static Vector<Path> generateCode(const ECSData& data, Path directory);

	private:
		static bool writeFile(const Path& path, gsl::span<const char> data, bool stub);
		static void writeFiles(const Path& directory, const CodeGenResult& files, Stats& stats);
		static int getHeaderVersion(gsl::span<const char> data, size_t& endOfHeader);
	};
}