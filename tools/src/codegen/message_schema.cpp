#include <yaml-cpp/yaml.h>
#include "message_schema.h"

using namespace Halley;

MessageSchema::MessageSchema() {}

MessageSchema::MessageSchema(YAML::Node node)
{
	name = node["message"].as<std::string>();

	for (auto& memberEntry : node["members"]) {
		for (auto m = memberEntry.begin(); m != memberEntry.end(); ++m) {
			members.emplace_back(VariableSchema(TypeSchema(m->second.as<std::string>()), m->first.as<std::string>()));
		}
	}
}
