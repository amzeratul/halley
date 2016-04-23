#include <yaml-cpp/yaml.h>
#include "system_schema.h"

using namespace Halley;

SystemSchema::SystemSchema() {}

SystemSchema::SystemSchema(YAML::Node node)
{
	name = node["system"].as<std::string>();

	if (node["families"].IsDefined()) {
		for (auto familyEntry : node["families"]) {
			for (auto iter = familyEntry.begin(); iter != familyEntry.end(); ++iter) {
				FamilySchema family;
				family.name = iter->first.as<std::string>();
				for (auto comp: iter->second) {
					auto splitStr = String(comp.as<std::string>()).split(' ');

					ComponentReferenceSchema component;
					if (splitStr.size() != 2 || (splitStr[0] != "write" && splitStr[0] != "read")) {
						throw Exception("Invalid family declaration, must be \"read compName\" or \"write compName\".");
					}
					component.write = splitStr[0] == "write";
					component.name = splitStr[1];

					family.components.push_back(component);
				}

				families.push_back(family);
			}
		}
	}

	String methodStr = node["method"].as<std::string>("update");
	if (methodStr == "update") {
		method = SystemMethod::Update;
	} else if (methodStr == "render") {
		method = SystemMethod::Render;
	} else {
		throw Exception("Unknown method type: " + methodStr);
	}

	String strategyStr = node["strategy"].as<std::string>("individual");
	if (strategyStr == "individual") {
		strategy = SystemStrategy::Individual;
	} else if (strategyStr == "global") {
		strategy = SystemStrategy::Global;
	} else if (strategyStr == "parallel") {
		strategy = SystemStrategy::Parallel;
	} else {
		throw Exception("Unknown strategy type: " + strategyStr);
	}

	smearing = node["smearing"].as<int>(1);

	// TODO: access

	// TODO: language
}
