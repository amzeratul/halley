#include "../yaml/halley-yamlcpp.h"
#include <halley/support/exception.h>
#include <halley/tools/ecs/system_schema.h>

using namespace Halley;

MessageReferenceSchema::MessageReferenceSchema(String name, String parameter)
	: name(name)
{
	receive = parameter == "receive";
	send = parameter == "send";
}

SystemSchema::SystemSchema() {}

SystemSchema::SystemSchema(YAML::Node node, bool generate)
	: generate(generate)
{
	name = node["name"].as<std::string>();

	if (node["families"].IsDefined()) {
		for (auto familyEntry : node["families"]) {
			for (auto iter = familyEntry.begin(); iter != familyEntry.end(); ++iter) {
				FamilySchema family;
				family.name = iter->first.as<std::string>();
				for (auto compList : iter->second) {
					for (auto comp = compList.begin(); comp != compList.end(); ++comp) {
						ComponentReferenceSchema component;
						String desc = comp->second.as<std::string>();
						bool read = false;
						bool write = false;
						for (auto& d : desc.split(' ')) {
							if (d == "write") {
								component.write = true;
								write = true;
							} else if (d == "read") {
								component.write = false;
								read = true;
							} else if (d == "optional") {
								component.optional = true;
							} else {
								throw Exception("Unknown component descriptor: " + d + ", in family " + name, HalleyExceptions::Resources);
							}
						}
						if (!read && !write) {
							throw Exception("Component must be either read or write, in family " + name, HalleyExceptions::Resources);
						}
						if (read && write) {
							throw Exception("Component mark as read and write, simply tag it write, in family " + name, HalleyExceptions::Resources);
						}
						component.write = desc.contains("write");
						component.optional = desc.contains("optional");
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
		throw Exception("Unknown method type: " + methodStr, HalleyExceptions::Resources);
	}

	String strategyStr = node["strategy"].as<std::string>(families.empty() ? "global" : "individual");
	if (strategyStr == "global") {
		strategy = SystemStrategy::Global;
	} else {
		if (families.empty()) {
			throw Exception("Systems with no families must use the global strategy.", HalleyExceptions::Resources);
		}

		if (strategyStr == "individual") {
			strategy = SystemStrategy::Individual;
		} else if (strategyStr == "parallel") {
			strategy = SystemStrategy::Parallel;
		} else {
			throw Exception("Unknown strategy type: " + strategyStr, HalleyExceptions::Resources);
		}
	}

	smearing = node["smearing"].as<int>(1);

	if (node["access"].IsDefined()) {
		int accessValue = 0;
		for(auto accessOpt : node["access"]) {
			String nodeName = accessOpt.as<std::string>();
			if (nodeName == "api") {
				accessValue |= int(SystemAccess::API);
			} else if (nodeName == "world") {
				accessValue |= int(SystemAccess::World);
			} else if (nodeName == "resources") {
				accessValue |= int(SystemAccess::Resources);
			} else {
				throw Exception("Unknown access type: " + nodeName, HalleyExceptions::Resources);
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
			throw Exception("Unknown language: " + lang, HalleyExceptions::Resources);
		}
	}

	if (node["messages"].IsDefined()) {
		for (auto messageEntry : node["messages"]) {
			for (auto iter = messageEntry.begin(); iter != messageEntry.end(); ++iter) {
				messages.push_back(MessageReferenceSchema(iter->first.as<std::string>(), iter->second.as<std::string>()));
			}
		}
	}

	if (node["systemMessages"].IsDefined()) {
		for (auto messageEntry : node["systemMessages"]) {
			for (auto iter = messageEntry.begin(); iter != messageEntry.end(); ++iter) {
				systemMessages.emplace_back(iter->first.as<std::string>(), iter->second.as<std::string>());
			}
		}
	}

	if (node["services"].IsDefined()) {
		for (auto serviceEntry : node["services"]) {
			ServiceSchema service;
			service.name = serviceEntry.as<std::string>();
			services.push_back(service);
		}
	}
}

bool SystemSchema::operator<(const SystemSchema& other) const
{
	return name < other.name;
}
