#include "metadata.h"
#include "resource_data.h"

#ifdef _MSC_VER
#pragma warning(disable: 4127)
#endif
#include <yaml-cpp/yaml.h>

using namespace Halley;

Metadata::Metadata()
	: root(std::make_unique<YAML::Node>())
{}

Metadata::Metadata(const ResourceDataStatic& data)
	: root(std::make_unique<YAML::Node>())
{
	*root = YAML::Load(data.getString());
}

Metadata::~Metadata() {}

bool Metadata::getBool(String key) const
{
	return (*root)[key.cppStr()].as<bool>();
}

int Metadata::getInt(String key) const
{
	return (*root)[key.cppStr()].as<int>();
}

float Metadata::getFloat(String key) const
{
	return (*root)[key.cppStr()].as<float>();
}

String Metadata::getString(String key) const
{
	return (*root)[key.cppStr()].as<std::string>();
}

bool Metadata::getBool(String key, bool v) const
{
	return (*root)[key.cppStr()].as<bool>(v);
}

int Metadata::getInt(String key, int v) const
{
	return (*root)[key.cppStr()].as<int>(v);
}

float Metadata::getFloat(String key, float v) const
{
	return (*root)[key.cppStr()].as<float>(v);
}

String Metadata::getString(String key, String v) const
{
	return (*root)[key.cppStr()].as<std::string>(v);
}
