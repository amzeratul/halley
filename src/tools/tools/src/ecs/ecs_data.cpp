#include "halley/tools/ecs/ecs_data.h"

#include <yaml-cpp/yaml.h>

#include "halley/support/exception.h"
#include "halley/text/string_converter.h"
#include "halley/tools/ecs/system_message_schema.h"
using namespace Halley;


bool CodegenSourceInfo::operator<(const CodegenSourceInfo& other) const
{
	return filename < other.filename;
}

void ECSData::loadSources(Vector<CodegenSourceInfo> files, bool throwOnValidationError)
{
	++revision;

	std::sort(files.begin(), files.end());
	for (auto& f : files) {
		addSource(f);
	}

	if (throwOnValidationError) {
		const auto error = validate();
		if (error) {
			throw Exception(*error, HalleyExceptions::Tools);
		}
	}
	
	process();
}

const HashMap<String, ComponentSchema>& ECSData::getComponents() const
{
	return components;
}

const HashMap<String, SystemSchema>& ECSData::getSystems() const
{
	return systems;
}

const HashMap<String, MessageSchema>& ECSData::getMessages() const
{
	return messages;
}

const HashMap<String, SystemMessageSchema>& ECSData::getSystemMessages() const
{
	return systemMessages;
}

const HashMap<String, CustomTypeSchema>& ECSData::getCustomTypes() const
{
	return types;
}

void ECSData::clear()
{
	++revision;
	components.clear();
	systems.clear();
	messages.clear();
	systemMessages.clear();
	types.clear();
}

int ECSData::getRevision() const
{
	return revision;
}

std::optional<String> ECSData::validate()
{
	for (auto& comp: components) {
		for (auto& dep: comp.second.componentDependencies) {
			if (dep == comp.second.name) {
				return "Component " + comp.second.name + " depends on itself.";
			} else if (components.find(dep) == components.end()) {
				return "Component " + comp.second.name + " depends on missing component \"" + dep + "\"";
			}
		}
	}
	
	for (auto& sys : systems) {
		bool hasMain = false;
		std::set<String> famNames;

		for (auto& fam : sys.second.families) {
			if (fam.name == "main") {
				hasMain = true;
			}
			if (famNames.find(fam.name) != famNames.end()) {
				return "System " + sys.second.name + " already has a family named " + fam.name;
			}
			famNames.emplace(fam.name);

			for (auto& comp : fam.components) {
				if (components.find(comp.name) == components.end()) {
					return "Unknown component \"" + comp.name + "\" in family \"" + fam.name + "\" of system \"" + sys.second.name + "\".";
				}
			}
		}

		for (auto& msg : sys.second.messages) {
			if (messages.find(msg.name) == messages.end()) {
				return "Unknown message \"" + msg.name + "\" in system \"" + sys.second.name + "\".";
			}
		}

		if (!sys.second.systemMessages.empty() && sys.second.method == SystemMethod::Render) {
			return "Render system \"" + sys.second.name + "\" cannot send or receive system messages.";
		}
		
		for (auto& msg : sys.second.systemMessages) {
			if (systemMessages.find(msg.name) == systemMessages.end()) {
				return "Unknown system message \"" + msg.name + "\" in system \"" + sys.second.name + "\".";
			}
		}

		for (auto& service : sys.second.services) {
			if (types.find(service.name) == types.end()) {
				return "Unknown service \"" + service.name + "\" in system \"" + sys.second.name + "\".";
			}
		}

		if (sys.second.strategy == SystemStrategy::Individual || sys.second.strategy == SystemStrategy::Parallel) {
			if (!hasMain) {
				return "System " + sys.second.name + " needs to have a main family due to its strategy.";
			}
			if (sys.second.strategy == SystemStrategy::Parallel) {
				if (sys.second.families.size() != 1) {
					return "System " + sys.second.name + " can only have one family due to its strategy.";
				}
			}
		}
	}

	return {};
}

void ECSData::process()
{
	for (auto& comp : components) {
		for (auto& m : comp.second.members) {
			String i = getInclude(m.type.name);
			if (i != "") {
				comp.second.includeFiles.insert(i);
			}
		}
	}
	
	for (auto& msg : messages) {
		for (auto& m : msg.second.members) {
			String i = getInclude(m.type.name);
			if (i != "") {
				msg.second.includeFiles.insert(i);
			}
		}
	}

	for (auto& msg : systemMessages) {
		for (auto& m : msg.second.members) {
			String i = getInclude(m.type.name);
			if (i != "") {
				msg.second.includeFiles.insert(i);
			}
		}
	}

	for (auto& system : systems) {
		auto& sys = system.second;

		for (auto& fam : sys.families) {
			// Sorting the components ensures that different systems which use the same family will not corrupt memory by accessing them in different orders
			std::sort(fam.components.begin(), fam.components.end(), [](const ComponentReferenceSchema& a, const ComponentReferenceSchema& b) -> bool {
				return a.name < b.name;
			});
		}

		for (auto& service : sys.services) {
			sys.includeFiles.insert(getInclude(service.name));
		}
	}
}

void ECSData::addSource(CodegenSourceInfo info)
{
	String strData(reinterpret_cast<const char*>(info.data.data()), info.data.size());
	auto documents = YAML::LoadAll(strData.cppStr());

	for (auto document : documents) {
		String curPos = info.filename + ":" + toString(document.Mark().line) + ":" + toString(document.Mark().column);

		if (!document.IsDefined() || document.IsNull()) {
			throw Exception("Invalid document in stream.", HalleyExceptions::Tools);
		}

		if (document.IsScalar()) {
			throw Exception("YAML parse error in codegen definitions:\n\"" + document.as<std::string>() + "\"\nat " + curPos, HalleyExceptions::Tools);
		}
		else if (document["component"].IsDefined()) {
			addComponent(document, info.generate);
		}
		else if (document["system"].IsDefined()) {
			addSystem(document, info.generate);
		}
		else if (document["message"].IsDefined()) {
			addMessage(document, info.generate);
		}
		else if (document["systemMessage"].IsDefined()) {
			addSystemMessage(document, info.generate);
		}
		else if (document["type"].IsDefined()) {
			addType(document);
		}
		else {
			throw Exception("YAML parse error in codegen definitions: unknown type\nat " + curPos, HalleyExceptions::Tools);
		}
	}
}

void ECSData::addComponent(YAML::Node rootNode, bool generate)
{
	auto comp = ComponentSchema(rootNode["component"], generate);

	if (components.find(comp.name) == components.end()) {
		comp.id = int(components.size());
		components[comp.name] = comp;
	}
	else {
		throw Exception("Component already declared: " + comp.name, HalleyExceptions::Tools);
	}
}

void ECSData::addSystem(YAML::Node rootNode, bool generate)
{
	auto sys = SystemSchema(rootNode["system"], generate);

	if (systems.find(sys.name) == systems.end()) {
		systems[sys.name] = sys;
	}
	else {
		throw Exception("System already declared: " + sys.name, HalleyExceptions::Tools);
	}
}

void ECSData::addMessage(YAML::Node rootNode, bool generate)
{
	auto msg = MessageSchema(rootNode["message"], generate);

	if (messages.find(msg.name) == messages.end()) {
		msg.id = int(messages.size());
		messages[msg.name] = msg;
	} else {
		throw Exception("Message already declared: " + msg.name, HalleyExceptions::Tools);
	}
}

void ECSData::addSystemMessage(YAML::Node rootNode, bool generate)
{
	auto msg = SystemMessageSchema(rootNode["systemMessage"], generate);

	if (systemMessages.find(msg.name) == systemMessages.end()) {
		msg.id = int(systemMessages.size());
		systemMessages[msg.name] = msg;
	} else {
		throw Exception("System message already declared: " + msg.name, HalleyExceptions::Tools);
	}
}

void ECSData::addType(YAML::Node rootNode)
{
	auto t = CustomTypeSchema(rootNode["type"]);

	if (types.find(t.name) == types.end()) {
		types[t.name] = t;
	}
	else {
		throw Exception("Type already declared: " + t.name, HalleyExceptions::Tools);
	}
}

String ECSData::getInclude(String typeName) const
{
	auto i = types.find(typeName);
	if (i != types.end()) {
		return i->second.includeFile;
	}
	return "";
}