#include <yaml-cpp/yaml.h>
#include "component_schema.h"

using namespace Halley;

ComponentSchema::ComponentSchema() {}

ComponentSchema::ComponentSchema(YAML::Node node)
{
	name = node["component"].as<std::string>();

	for (auto& memberEntry : node["members"]) {
		for (auto m = memberEntry.begin(); m != memberEntry.end(); ++m) {
			ComponentMemberSchema member;

			member.name = m->first.as<std::string>();
			member.type = m->second.as<std::string>();
			member.isConst = false;

			members.push_back(member);
		}
	}
}
