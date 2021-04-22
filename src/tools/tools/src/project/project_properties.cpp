#include <utility>
#include "halley/tools/project/project_properties.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file_formats/yaml_convert.h"
using namespace Halley;

ProjectProperties::ProjectProperties(Path propertiesFile)
	: propertiesFile(std::move(propertiesFile))
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

const String& ProjectProperties::getBinName() const
{
	return binName;
}

void ProjectProperties::setBinName(String binName)
{
	this->binName = std::move(binName);
}

bool ProjectProperties::getImportByExtension() const
{
	return importByExtension;
}

void ProjectProperties::setImportByExtension(bool enabled)
{
	importByExtension = enabled;
}

void ProjectProperties::setDefaultZoom(float zoom)
{
	defaultZoom = zoom;
}

float ProjectProperties::getDefaultZoom() const
{
	return defaultZoom;
}

void ProjectProperties::load()
{
	const auto data = FileSystem::readFile(propertiesFile);
	if (data.empty()) {
		return;
	}

	auto file = YAMLConvert::parseConfig(data);
	const auto& node = file.getRoot();

	name = node["name"].asString("Halley Project");
	assetPackManifest = node["assetPackManifest"].asString("halley_project/asset_manifest.yaml");
	binName = node["binName"].asString("");
	importByExtension = node["importByExtension"].asBool(false);
	defaultZoom = node["defaultZoom"].asFloat(1.0f);

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
