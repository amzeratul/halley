#include <yaml-cpp/yaml.h>
#include "halley/tools/codegen/custom_type_schema.h"

using namespace Halley;

CustomTypeSchema::CustomTypeSchema() {}

CustomTypeSchema::CustomTypeSchema(YAML::Node node)
{
	name = node["name"].as<std::string>();
	namespaceName = node["namespace"].as<std::string>("");
	includeFile = node["include"].as<std::string>();
}
