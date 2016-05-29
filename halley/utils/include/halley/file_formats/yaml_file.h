#pragma once
#include "halley/resources/resource.h"
#include <memory>

namespace YAML
{
	class Node;
}

namespace Halley
{
	class String;
	class ResourceLoader;

	class YAMLFile : public Resource
	{
	public:
		explicit YAMLFile(String string);
		explicit YAMLFile(std::unique_ptr<YAML::Node> node);
		~YAMLFile();
		YAML::Node& getRoot() const;
		
		static std::unique_ptr<YAMLFile> loadResource(ResourceLoader& loader);

	private:
		std::unique_ptr<YAML::Node> root;
	};
}
