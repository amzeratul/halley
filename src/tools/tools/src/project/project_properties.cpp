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

const String& ProjectProperties::getAssetPackManifest() const
{
	return assetPackManifest;
}

void ProjectProperties::setAssetPackManifest(String manifest)
{
	assetPackManifest = std::move(manifest);
}

const std::vector<String>& ProjectProperties::getPlatforms() const
{
	return platforms;
}

void ProjectProperties::setPlatforms(std::vector<String> platforms)
{
	this->platforms = std::move(platforms);
}

const String& ProjectProperties::getDLL() const
{
	return dll;
}

void ProjectProperties::setDLL(String dll)
{
	this->dll = std::move(dll);
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
	assetPackManifest = node["assetPackManifest"].asString("halley_project/asset_manifest.yaml");
	dll = node["dll"].asString("");

	if (node.hasKey("platforms")) {
		for (auto& plat: node["platforms"].asSequence()) {
			platforms.push_back(plat.asString());
		}
	} else {
		platforms = { "pc" };
	}
}

void ProjectProperties::save()
{
	// TODO
}
