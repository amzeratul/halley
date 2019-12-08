#include <utility>
#include "halley/tools/project/project_properties.h"
#include "halley/tools/file/filesystem.h"
#include "../yaml/halley-yamlcpp.h"
#include "../assets/importers/config_importer.h"
using namespace Halley;

ProjectProperties::ProjectProperties(const Path& propertiesFile)
	: propertiesFile(propertiesFile)
{
	load();
}

const String& ProjectProperties::getName() const
{
	return name;
}

void ProjectProperties::setName(String n)
{
	name = std::move(n);
}

void ProjectProperties::load()
{
	auto data = FileSystem::readFile(propertiesFile);
	if (data.empty()) {
		return;
	}

	String strData = String(reinterpret_cast<const char*>(data.data()), data.size());
	YAML::Node root = YAML::Load(strData);
	auto node = ConfigImporter::parseYAMLNode(root);

	name = node["name"].asString("Halley Project");
}

void ProjectProperties::save()
{
	// TODO
}
