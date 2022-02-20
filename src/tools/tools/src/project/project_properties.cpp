#include <utility>
#include "halley/tools/project/project_properties.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/maths/uuid.h"
#include "halley/support/logger.h"
using namespace Halley;

ProjectProperties::ProjectProperties(Path propertiesFile)
	: propertiesFile(std::move(propertiesFile))
{
	load();
}

const UUID& ProjectProperties::getUUID() const
{
	return uuid;
}

const String& ProjectProperties::getName() const
{
	return name;
}

void ProjectProperties::setName(String n)
{
	name = std::move(n);
	dirty = true;
}

const String& ProjectProperties::getAssetPackManifest() const
{
	return assetPackManifest;
}

void ProjectProperties::setAssetPackManifest(String manifest)
{
	assetPackManifest = std::move(manifest);
	dirty = true;
}

const Vector<String>& ProjectProperties::getPlatforms() const
{
	return platforms;
}

void ProjectProperties::setPlatforms(Vector<String> platforms)
{
	this->platforms = std::move(platforms);
	dirty = true;
}

const String& ProjectProperties::getBinName() const
{
	return binName;
}

void ProjectProperties::setBinName(String binName)
{
	this->binName = std::move(binName);
	dirty = true;
}

bool ProjectProperties::getImportByExtension() const
{
	return importByExtension;
}

void ProjectProperties::setImportByExtension(bool enabled)
{
	importByExtension = enabled;
	dirty = true;
}

void ProjectProperties::setDefaultZoom(float zoom)
{
	defaultZoom = zoom;
	dirty = true;
}

float ProjectProperties::getDefaultZoom() const
{
	return defaultZoom;
}

void ProjectProperties::loadDefaults()
{
	uuid = UUID::generate();
	name = "Halley Project";
	assetPackManifest = "halley_project/asset_manifest.yaml";
	binName = "";
	importByExtension = false;
	defaultZoom = 1.0f;
	platforms = {"pc"};
}

void ProjectProperties::load()
{
	loadDefaults();
	
	const auto data = Path::readFile(propertiesFile);
	if (!data.empty()) {
		auto file = YAMLConvert::parseConfig(data);
		const auto& node = file.getRoot();

		if (node.hasKey("uuid")) {
			uuid = UUID(node["uuid"].asString());
		}
		if (node.hasKey("name")) {
			name = node["name"].asString();
		}
		if (node.hasKey("assetPackManifest")) {
			assetPackManifest = node["assetPackManifest"].asString();
		}
		if (node.hasKey("binName")) {
			binName = node["binName"].asString();
		}
		if (node.hasKey("importByExtension")) {
			importByExtension = node["importByExtension"].asBool();
		}
		if (node.hasKey("defaultZoom")) {
			defaultZoom = node["defaultZoom"].asFloat();
		}
		if (node.hasKey("platforms")) {
			platforms = node["platforms"].asVector<String>();
		}
	}

	save();
}

void ProjectProperties::save()
{
	ConfigNode node = ConfigNode::MapType();

	node["uuid"] = uuid.toString();
	node["name"] = name;
	node["assetPackManifest"] = assetPackManifest;
	node["binName"] = binName;
	node["importByExtension"] = importByExtension;
	node["defaultZoom"] = defaultZoom;
	node["platforms"] = platforms;

	const auto curFile = Path::readFile(propertiesFile);
	const auto yaml = YAMLConvert::generateYAML(node, YAMLConvert::EmitOptions());

	try {
		const auto newBytes = gsl::as_bytes(gsl::span<const char>(yaml.c_str(), yaml.length()));
		const auto oldBytes = gsl::as_bytes(gsl::span<const Byte>(curFile));
		if (!std::equal(newBytes.begin(), newBytes.end(), oldBytes.begin(), oldBytes.end())) {
			Path::writeFile(propertiesFile, yaml);
		}
		dirty = false;
	} catch (...) {
		Logger::logError("Unable to save preferences file.");
	}
}
