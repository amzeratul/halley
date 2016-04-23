#pragma once

#include <experimental/filesystem>
#include <map>
#include "icode_generator.h"

namespace YAML
{
	class Node;
}

namespace Halley
{
	class ComponentSchema;
	class SystemSchema;

	class Codegen
	{
	public:
		static void run(String inDir, String outDir);

		void loadSources(String directory);
		void validate();
		void process();
		bool writeFile(String path, const char* data, size_t dataSize);
		void writeFiles(String directory, const CodeGenResult& files);
		void generateCode(String directory);

	private:
		void addSource(std::experimental::filesystem::path path);
		void addComponent(YAML::Node rootNode);
		void addSystem(YAML::Node rootNode);

		std::map<String, ComponentSchema> components;
		std::map<String, SystemSchema> systems;
	};
}