#pragma once

#include <boost/filesystem.hpp>
#include <map>
#include "../../../../src/codegen/icode_generator.h"

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
		static void run(String inDir, String outDir);

		void loadSources(String directory);
		void validate();
		void process();
		bool writeFile(String path, const char* data, size_t dataSize) const;
		void writeFiles(String directory, const CodeGenResult& files, Stats& stats);
		void generateCode(String directory);

	private:
		void addSource(boost::filesystem::path path);
		void addComponent(YAML::Node rootNode);
		void addSystem(YAML::Node rootNode);
		void addMessage(YAML::Node rootNode);

		std::map<String, ComponentSchema> components;
		std::map<String, SystemSchema> systems;
		std::map<String, MessageSchema> messages;
	};
}