#pragma once

#include "icode_generator.h"
#include <halley/data_structures/hash_map.h>
#include "halley/file/filesystem.h"
#include <gsl/gsl>

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
			std::vector<Path> files;
		};

		static bool doNothing(float, String) { return true; }

	public:
		using ProgressReporter = std::function<bool(float, String)>;

		static void run(Path inDir, Path outDir);

		explicit Codegen(bool verbose = false);

		void loadSources(std::vector<std::pair<String, gsl::span<const gsl::byte>>> files, ProgressReporter progress = &doNothing);
		void validate(ProgressReporter progress = &doNothing);
		void process();
		bool writeFile(Path path, const char* data, size_t dataSize, bool stub) const;
		void writeFiles(Path directory, const CodeGenResult& files, Stats& stats) const;
		std::vector<Path> generateCode(Path directory, ProgressReporter progress = &doNothing);

	private:
		void addSource(String name, gsl::span<const gsl::byte> data);
		void addComponent(YAML::Node rootNode);
		void addSystem(YAML::Node rootNode);
		void addMessage(YAML::Node rootNode);

		bool verbose;
		HashMap<String, ComponentSchema> components;
		HashMap<String, SystemSchema> systems;
		HashMap<String, MessageSchema> messages;
	};
}