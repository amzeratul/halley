#pragma once

#include "icode_generator.h"
#include <halley/data_structures/hash_map.h>
#include "halley/file/filesystem.h"

namespace YAML
{
	class Node;
}

namespace Halley
{
	class ComponentSchema;
	class SystemSchema;
	class MessageSchema;

	class Codegen
	{
		struct Stats
		{
			int written = 0;
			int skipped = 0;
		};

	public:
		static void run(Path inDir, Path outDir);

		void loadSources(Path directory);
		void validate();
		void process();
		bool writeFile(Path path, const char* data, size_t dataSize, bool stub) const;
		void writeFiles(Path directory, const CodeGenResult& files, Stats& stats) const;
		void generateCode(Path directory);

	private:
		void addSource(Path path);
		void addComponent(YAML::Node rootNode);
		void addSystem(YAML::Node rootNode);
		void addMessage(YAML::Node rootNode);

		HashMap<String, ComponentSchema> components;
		HashMap<String, SystemSchema> systems;
		HashMap<String, MessageSchema> messages;
	};
}