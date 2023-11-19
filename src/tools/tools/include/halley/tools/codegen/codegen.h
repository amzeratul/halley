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
	class IAssetCollector;
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
		constexpr static int currentCodegenVersion = 127;
		
		using ProgressReporter = std::function<bool(float, String)>;

		static void run(Path inDir, Path outDir);
		static Vector<Path> generateCode(const ECSData& data, Path directory, IAssetCollector* collector);

	private:
		static bool needsToWriteFile(const Path& path, gsl::span<const char> data, bool stub);
		static void writeFiles(const Path& outputDir, const Path& prefix, const CodeGenResult& files, Stats& stats, IAssetCollector* collector);
		static int getHeaderVersion(gsl::span<const char> data, size_t& endOfHeader);
	};
}