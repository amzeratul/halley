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
	class ComponentSchema;
	class SystemSchema;
	class MessageSchema;
	class CustomTypeSchema;

	struct CodegenSourceInfo {
		String filename;
		gsl::span<const gsl::byte> data;
		bool generate = false;
	};
	
	class Codegen
	{	
		struct Stats
		{
			int written = 0;
			int skipped = 0;
			std::vector<Path> files;
		};

		static bool doNothing(float, String) { return true; }

	public:
		using ProgressReporter = std::function<bool(float, String)>;

		static void run(Path inDir, Path outDir);

		explicit Codegen(bool verbose = false);

		void loadSources(std::vector<CodegenSourceInfo> files, ProgressReporter progress = &doNothing);
		void validate(ProgressReporter progress = &doNothing);
		void process();
		bool writeFile(Path path, const char* data, size_t dataSize, bool stub) const;
		void writeFiles(Path directory, const CodeGenResult& files, Stats& stats) const;
		std::vector<Path> generateCode(Path directory, ProgressReporter progress = &doNothing);

	private:
		void addSource(CodegenSourceInfo sourceInfo);
		void addComponent(YAML::Node rootNode, bool generate);
		void addSystem(YAML::Node rootNode, bool generate);
		void addMessage(YAML::Node rootNode, bool generate);
		void addType(YAML::Node rootNode);
		String getInclude(String typeName) const;

		bool verbose;
		HashMap<String, ComponentSchema> components;
		HashMap<String, SystemSchema> systems;
		HashMap<String, MessageSchema> messages;
		HashMap<String, CustomTypeSchema> types;
	};
}