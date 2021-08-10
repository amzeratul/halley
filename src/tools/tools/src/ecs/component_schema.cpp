#include "halley/file_formats/halley-yamlcpp.h"
#include <halley/tools/ecs/component_schema.h>

#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

using namespace Halley;

ComponentSchema::ComponentSchema() {}

ComponentSchema::ComponentSchema(YAML::Node node, bool generate)
	: generate(generate)
{
	name = node["name"].as<std::string>();

	for (auto memberEntry : node["members"]) {
		for (auto m = memberEntry.begin(); m != memberEntry.end(); ++m) {
			String name = m->first.as<std::string>();
			if (m->second.IsScalar()) {
				// e.g. - value: int
				members.emplace_back(TypeSchema(m->second.as<std::string>()), std::move(name));
			} else {
				// e.g.
				// value:
				//   type: int
				//   access: protected
				//   initialValue: 5
				const YAML::Node& memberProperties = m->second;
				const String type = memberProperties["type"].as<std::string>();
				const String access = memberProperties["access"].as<std::string>("public");
				const String displayName = memberProperties["displayName"].as<std::string>("");
				const bool canEdit = memberProperties["canEdit"].as<bool>(true);
				const bool canSave = memberProperties["canSave"].as<bool>(true);
				const bool collapse = memberProperties["collapse"].as<bool>(false);

				if (memberProperties["serializable"].IsDefined()) {
					throw Exception("serializable field is removed from ECS component definitions. Use canSave and canEdit instead.", HalleyExceptions::Entity);
				}

				std::vector<String> defaultValue;
				const auto& defNode = memberProperties["defaultValue"];
				if (defNode.IsDefined()) {
					if (defNode.IsSequence()) {
						for (const auto& d: defNode) {
							defaultValue.emplace_back(d.as<std::string>());
						}
					} else {
						defaultValue.emplace_back(defNode.as<std::string>());
					}
				}
				
				auto& field = members.emplace_back(TypeSchema(type), std::move(name), std::move(defaultValue), fromString<MemberAccess>(access));
				field.collapse = collapse;
				field.canEdit = canEdit;
				field.canSave = canSave;
				field.displayName = displayName;
			}
		}
	}

	if (node["customImplementation"].IsDefined()) {
		customImplementation = node["customImplementation"].as<std::string>();
	}

	const auto deps = node["componentDependencies"];
	if (deps.IsSequence()) {
		for (auto n = deps.begin(); n != deps.end(); ++n) {
			if (n->IsScalar()) {
				componentDependencies.push_back(n->as<std::string>());
			}
		}
	}
}

bool ComponentSchema::operator<(const ComponentSchema& other) const
{
	return id < other.id;
}
