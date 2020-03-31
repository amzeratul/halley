#include "../yaml/halley-yamlcpp.h"
#include <halley/tools/ecs/component_schema.h>
#include "halley/text/string_converter.h"

using namespace Halley;

ComponentSchema::ComponentSchema() {}

ComponentSchema::ComponentSchema(YAML::Node node, bool generate)
	: generate(generate)
{
	name = node["name"].as<std::string>();

	for (auto memberEntry : node["members"]) {
		for (auto m = memberEntry.begin(); m != memberEntry.end(); ++m) {
			const String name = m->first.as<std::string>();
			if (m->second.IsScalar()) {
				// e.g. - value: int
				members.emplace_back(TypeSchema(m->second.as<std::string>()), name);
			} else {
				// e.g.
				// value:
				//   type: int
				//   access: protected
				//   initialValue: 5
				const YAML::Node& memberProperties = m->second;
				const String type = memberProperties["type"].as<std::string>();
				const String access = memberProperties["access"].as<std::string>("public");
				const String defaultValue = memberProperties["defaultValue"].as<std::string>("");
				members.emplace_back(TypeSchema(type), name, defaultValue, fromString<MemberAccess>(access));
			}
		}
	}

	if (node["customImplementation"].IsDefined()) {
		customImplementation = node["customImplementation"].as<std::string>();
	}
}
