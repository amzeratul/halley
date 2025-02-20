#include "halley/file_formats/halley-yamlcpp.h"
#include <halley/tools/ecs/message_schema.h>

using namespace Halley;

MessageSchema::MessageSchema() {}

MessageSchema::MessageSchema(YAML::Node node, bool generate)
	: generate(generate)
{
	name = node["name"].as<std::string>();
	serializable = node["serializable"].as<bool>(false);

	for (auto memberEntry : node["members"]) {
		for (auto m = memberEntry.begin(); m != memberEntry.end(); ++m) {
			if (m->second.IsMap()) {
				auto type = m->second["type"].as<std::string>();
				auto defaultValue = String(m->second["defaultValue"].as<std::string>(""));
				members.emplace_back(TypeSchema(type), m->first.as<std::string>(), Vector<String>({ defaultValue }));
			} else {
				members.emplace_back(TypeSchema(m->second.as<std::string>()), m->first.as<std::string>());
			}
		}
	}
}

bool MessageSchema::operator<(const MessageSchema& other) const
{
	return id < other.id;
}
