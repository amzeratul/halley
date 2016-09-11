#include "halley/file_formats/yaml_file.h"
#include "halley/resources/resource_data.h"

#ifdef _MSC_VER
#pragma warning(disable: 4127)
#pragma comment(lib, "libyaml-cppmd.lib")
#endif

#include <yaml-cpp/yaml.h>

using namespace Halley;

YAMLFile::YAMLFile(String string)
{
	root = std::make_unique<YAML::Node>();
	*root = YAML::Load(string);
}

YAMLFile::YAMLFile(std::unique_ptr<YAML::Node> node)
	: root(std::move(node))
{
}

YAMLFile::~YAMLFile() = default;

YAML::Node& YAMLFile::getRoot() const
{
	return *root;
}

std::unique_ptr<YAMLFile> YAMLFile::loadResource(ResourceLoader& loader)
{
	return std::make_unique<YAMLFile>(loader.getStatic()->getString());
}
