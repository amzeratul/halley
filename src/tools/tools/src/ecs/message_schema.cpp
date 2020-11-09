#include "halley/file_formats/halley-yamlcpp.h"
#include <halley/tools/ecs/message_schema.h>

using namespace Halley;

MessageSchema::MessageSchema() {}

MessageSchema::MessageSchema(YAML::Node node, bool generate)
	: generate(generate)
{
	name = node["name"].as<std::string>();

	for (auto memberEntry : node["members"]) {
		for (auto m = memberEntry.begin(); m != memberEntry.end(); ++m) {
			members.emplace_back(TypeSchema(m->second.as<std::string>()), m->first.as<std::string>());
		}
	}
}
