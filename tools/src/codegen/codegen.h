#pragma once

#include <experimental/filesystem>
#include <map>

namespace YAML
{
	class Node;
}

namespace Halley
{
	class ComponentSchema;
	class SystemSchema;

	enum class CodegenLanguage
	{
		CPlusPlus,
		Lua
	};

	class Codegen
	{
	public:
		static void run(String inDir, String outDir);

		void loadSources(String directory);
		void process();
		void generateCode(String directory, CodegenLanguage language);

	private:
		void addSource(std::experimental::filesystem::path path);
		void addComponent(YAML::Node rootNode);
		void addSystem(YAML::Node rootNode);

		std::map<String, ComponentSchema> components;
		std::map<String, SystemSchema> systems;
	};
}