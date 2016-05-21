#include <yaml-cpp/yaml.h>
#include <halley/support/exception.h>
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
				for (auto compList : iter->second) {
					for (auto comp = compList.begin(); comp != compList.end(); ++comp) {
						ComponentReferenceSchema component;
						component.write = comp->second.as<std::string>() == "write";
						component.name = comp->first.as<std::string>();

						family.components.push_back(component);
					}
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

	String strategyStr = node["strategy"].as<std::string>(families.empty() ? "global" : "individual");
	if (strategyStr == "global") {
		strategy = SystemStrategy::Global;
	} else {
		if (families.empty()) {
			throw Exception("Systems with no families must use the global strategy.");
		}

		if (strategyStr == "individual") {
			strategy = SystemStrategy::Individual;
		} else if (strategyStr == "parallel") {
			strategy = SystemStrategy::Parallel;
		} else {
			throw Exception("Unknown strategy type: " + strategyStr);
		}
	}

	smearing = node["smearing"].as<int>(1);

	if (node["access"].IsDefined()) {
		int accessValue = 0;
		for(auto accessOpt : node["access"]) {
			String name = accessOpt.as<std::string>();
			if (name == "api") {
				accessValue |= int(SystemAccess::API);
			} else if (name == "world") {
				accessValue |= int(SystemAccess::World);
			} else {
				throw Exception("Unknown access type: " + name);
			}
		}
		access = SystemAccess(accessValue);
	}

	if (node["language"].IsDefined()) {
		String lang = node["language"].as<std::string>();
		if (lang == "cpp") {
			language = CodegenLanguage::CPlusPlus;
		} else if (lang == "lua") {
			language = CodegenLanguage::Lua;
		} else {
			throw Exception("Unknown language: " + lang);
		}
	}

	if (node["messages"].IsDefined()) {
		for (auto messageEntry : node["messages"]) {
			for (auto iter = messageEntry.begin(); iter != messageEntry.end(); ++iter) {
				MessageReferenceSchema msg;
				msg.name = iter->first.as<std::string>();
				String type = iter->second.as<std::string>();
				msg.receive = type == "receive";
				msg.send = type == "send";
				messages.push_back(msg);
			}
		}
	}
}
