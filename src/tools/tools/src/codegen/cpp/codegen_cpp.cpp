#include <sstream>
#include "codegen_cpp.h"
#include "cpp_class_gen.h"
#include <set>
#include <halley/support/exception.h>
#include <algorithm>
#include "halley/text/string_converter.h"
#include "halley/core/game/game_platform.h"

using namespace Halley;

static String upperFirst(String name)
{
	if (name[0] >= 'a' && name[0] <= 'z') {
		name[0] -= 32;
	}
	return name;
}

static String lowerFirst(String name)
{
	if (name[0] >= 'A' && name[0] <= 'Z') {
		name[0] += 32;
	}
	return name;
}

CodeGenResult CodegenCPP::generateComponent(ComponentSchema component)
{
	const String className = component.name + "Component" + (component.customImplementation ? "Base" : "");

	CodeGenResult result;
	result.emplace_back(CodeGenFile(makePath("components", className, "h"), generateComponentHeader(component)));
	return result;
}

CodeGenResult CodegenCPP::generateSystem(SystemSchema system, const HashMap<String, ComponentSchema>& components)
{
	const String className = system.name + "System";

	CodeGenResult result;
	result.emplace_back(CodeGenFile(makePath("systems", className, "h"), generateSystemHeader(system, components)));
	//result.emplace_back(CodeGenFile(makePath("../../src/systems", className, "cpp"), generateSystemStub(system), true));
	return result;
}

CodeGenResult CodegenCPP::generateMessage(MessageSchema message)
{
	const String className = message.name + "Message";

	CodeGenResult result;
	result.emplace_back(CodeGenFile(makePath("messages", className, "h"), generateMessageHeader(message)));
	return result;
}

CodeGenResult CodegenCPP::generateRegistry(const Vector<ComponentSchema>& components, const Vector<SystemSchema>& systems)
{
	Vector<String> registryCpp {
		"#include <halley.hpp>",
		"using namespace Halley;",
		""
	};

	for (auto& comp: components) {
		registryCpp.emplace_back("#include \"" + getComponentFileName(comp) + "\"");
	}
	
	registryCpp.insert(registryCpp.end(), {
		"",
		"// System factory functions"
	});

	for (auto& sys: systems) {
		registryCpp.push_back("System* halleyCreate" + sys.name + "System();");
	}

	// System factory
	registryCpp.insert(registryCpp.end(), {
		"",
		"",
		"using SystemFactoryPtr = System* (*)();",
		"using SystemFactoryMap = HashMap<String, SystemFactoryPtr>;",
		"",
		"static SystemFactoryMap makeSystemFactories() {",
		"	SystemFactoryMap result;"
	});

	for (auto& sys : systems) {
		registryCpp.push_back("	result[\"" + sys.name + "System\"] = &halleyCreate" + sys.name + "System;");
	}

	registryCpp.insert(registryCpp.end(), {
		"	return result;",
		"}"
	});

	// Component factory
	registryCpp.insert(registryCpp.end(), {
		"",
		"",
		"using ComponentFactoryPtr = std::function<void(EntityFactory&, EntityRef&, const ConfigNode&)>;",
		"using ComponentFactoryMap = HashMap<String, ComponentFactoryPtr>;",
		"",
		"static ComponentFactoryMap makeComponentFactories() {",
		"	ComponentFactoryMap result;"
	});

	for (auto& comp : components) {
		registryCpp.push_back("	result[\"" + comp.name + "\"] = [] (EntityFactory& factory, EntityRef& e, const ConfigNode& node) { factory.createComponent<" + comp.name + "Component>(e, node); };");
	}

	registryCpp.insert(registryCpp.end(), {
		"	return result;",
		"}"
	});

	// Create system and component methods
	registryCpp.insert(registryCpp.end(), {
		"",
		"namespace Halley {",
		"	std::unique_ptr<System> createSystem(String name) {",
		"		static SystemFactoryMap factories = makeSystemFactories();",
		"		auto result = factories.find(name);",
		"		if (result == factories.end()) {",
		"			throw Exception(\"System not found: \" + name, HalleyExceptions::Entity);",
		"		}",
		"		return std::unique_ptr<System>(result->second());",
		"	}",
		"",
		"   void createComponent(EntityFactory& factory, const String& name, EntityRef& entity, const ConfigNode& componentData) {",
		"		static ComponentFactoryMap factories = makeComponentFactories();",
		"		auto result = factories.find(name);",
		"		if (result == factories.end()) {",
		"			throw Exception(\"Component not found: \" + name, HalleyExceptions::Entity);",
		"		}",
		"		return result->second(factory, entity, componentData);",
		"   }",
		"}"
	});

	Vector<String> registryH{
		"#pragma once",
		"",
		"namespace Halley {",
		"	std::unique_ptr<System> createSystem(String name);",
		"	void createComponent(EntityFactory& factory, const String& name, EntityRef& entity, const ConfigNode& componentData);",
		"}"
	};

	CodeGenResult result;
	result.emplace_back(CodeGenFile("registry.cpp", registryCpp));
	result.emplace_back(CodeGenFile("registry.h", registryH));
	return result;
}

Vector<String> CodegenCPP::generateComponentHeader(ComponentSchema component)
{
	Vector<String> contents = {
		"#pragma once",
		"",
		"#ifndef DONT_INCLUDE_HALLEY_HPP",
		"#include <halley.hpp>",
		"#endif"
		""
	};

	for (auto& includeFile: component.includeFiles) {
		contents.push_back("#include \"" + includeFile + "\"");
	}
	contents.push_back("");

	String deserializeBody;
	bool first = true;
	for (auto& member: component.members) {
		if (first) {
			first = false;
		} else {
			if constexpr (getPlatform() == GamePlatform::Windows) {
				deserializeBody += "\r\n\t\t";
			} else {
				deserializeBody += "\n\t\t";
			}
		}
		deserializeBody += "Halley::ConfigNodeHelper::deserializeIfDefined(" + member.name + ", context, node[\"" + member.name + "\"]);";
	}

	String className = component.name + "Component" + (component.customImplementation ? "Base" : "");
	
	auto gen = CPPClassGenerator(className, "Halley::Component", MemberAccess::Public, !component.customImplementation)
		.addAccessLevelSection(MemberAccess::Public)
		.addMember(MemberSchema(TypeSchema("int", false, true, true), "componentIndex", toString(component.id)))
		.addBlankLine()
		.addMembers(component.members)
		.addBlankLine()
		.addAccessLevelSection(MemberAccess::Public)
		.addDefaultConstructor();

	// Additional constructors
	if (!component.members.empty()) {
		
		gen.addBlankLine()
			.addConstructor(MemberSchema::toVariableSchema(component.members));
	}
	
	// Deserialize method
	gen.addBlankLine()
		.addMethodDefinition(MethodSchema(TypeSchema("void"), { VariableSchema(TypeSchema("Halley::ConfigNodeSerializationContext&"), "context"), VariableSchema(TypeSchema("Halley::ConfigNode&", true), "node") }, "deserialize"), deserializeBody);

	gen.finish()
		.writeTo(contents);

	return contents;
}

template <typename T, typename U>
Vector<U> convert(Vector<T> in, U(*f)(const T&))
{
	size_t sz = in.size();
	Vector<U> result;
	result.reserve(sz);
	for (size_t i = 0; i < sz; i++) {
		result.emplace_back(f(in[i]));
	}
	return result;
}

class SystemInfo
{
public:
	SystemInfo(SystemSchema& system)
	{
		if (system.method == SystemMethod::Update) {
			methodName = "update";
			methodArgType = "Halley::Time";
			methodArgName = "time";
			methodConst = false;
		}
		else if (system.method == SystemMethod::Render) {
			methodName = "render";
			methodArgType = "Halley::RenderContext&";
			methodArgName = "rc";
			methodConst = true;
		}
		else {
			throw Exception("Unsupported method in " + system.name + "System", HalleyExceptions::Tools);
		}

		familyArgs = { VariableSchema(TypeSchema(methodArgType), methodArgName) };
		if (system.strategy == SystemStrategy::Global) {
			stratImpl = "static_cast<T*>(this)->" + methodName + "(" + methodArgName + ");";
		} else if (system.strategy == SystemStrategy::Individual) {
			familyArgs.push_back(VariableSchema(TypeSchema("MainFamily&"), "e"));
			stratImpl = "invokeIndividual([this, &" + methodArgName + "] (auto& e) { static_cast<T*>(this)->" + methodName + "(" + methodArgName + ", e); }, mainFamily);";
		} else if (system.strategy == SystemStrategy::Parallel) {
			familyArgs.push_back(VariableSchema(TypeSchema("MainFamily&"), "e"));
			stratImpl = "invokeParallel([this, &" + methodArgName + "] (auto& e) { static_cast<T*>(this)->" + methodName + "(" + methodArgName + ", e); }, mainFamily);";
		} else {
			throw Exception("Unsupported strategy in " + system.name + "System", HalleyExceptions::Tools);
		}
	}

	String methodName;
	String methodArgType;
	String methodArgName;
	String stratImpl;
	Vector<VariableSchema> familyArgs;
	bool methodConst;
};

Vector<String> CodegenCPP::generateSystemHeader(SystemSchema& system, const HashMap<String, ComponentSchema>& components) const
{
	auto info = SystemInfo(system);

	Vector<String> contents = {
		"#pragma once",
		"",
		"#include <halley.hpp>",
		""
	};

	// General headers
	for (auto& includeFile: system.includeFiles) {
		contents.push_back("#include \"" + includeFile + "\"");
	}
	contents.push_back("");

	// Family headers
	std::set<String> included;
	for (auto& fam : system.families) {
		for (auto& comp : fam.components) {
			if (included.find(comp.name) == included.end()) {
				auto iter = components.find(comp.name);
				if (iter != components.end()) {
					contents.emplace_back("#include \"" + getComponentFileName(iter->second) + "\"");
					included.emplace(comp.name);
				}
			}
		}
	}
	for (auto& msg : system.messages) {
		contents.emplace_back("#include \"../messages/" + toFileName(msg.name + "Message") + ".h\"");
	}

	contents.insert(contents.end(), {
		"",
		"// Generated file; do not modify.",
		"template <typename T>"
	});

	auto sysClassGen = CPPClassGenerator(system.name + "SystemBase", "Halley::System", MemberAccess::Private)
		.addMethodDeclaration(MethodSchema(TypeSchema("Halley::System*"), {}, "halleyCreate" + system.name + "System", false, false, false, false, true))
		.addBlankLine()
		.addAccessLevelSection(MemberAccess::Public);

	for (auto& fam : system.families) {
		auto members = convert<ComponentReferenceSchema, MemberSchema>(fam.components, [](auto& comp)
		{
			String type = comp.optional ? "Halley::MaybeRef<" + comp.name + "Component>" : comp.name + "Component&";
			return MemberSchema(TypeSchema(type, !comp.write), lowerFirst(comp.name));
		});

		sysClassGen
			.addClass(CPPClassGenerator(upperFirst(fam.name) + "Family", "Halley::FamilyBaseOf<" + upperFirst(fam.name) + "Family>")
				.addAccessLevelSection(MemberAccess::Public)
				.addMembers(members)
				.addBlankLine()
				.addTypeDefinition("Type", "Halley::FamilyType<" + String::concatList(convert<ComponentReferenceSchema, String>(fam.components, [](auto& comp)
				{
					return comp.optional ? "Halley::MaybeRef<" + comp.name + "Component>" : comp.name + "Component";
				}), ", ") + ">")
				.addBlankLine()
				.addAccessLevelSection(MemberAccess::Protected)
				.addConstructor(MemberSchema::toVariableSchema(members))
				.finish())
			.addBlankLine();
	}

	sysClassGen.addAccessLevelSection(MemberAccess::Protected);

	if ((int(system.access) & int(SystemAccess::API)) != 0) {
		sysClassGen.addMethodDefinition(MethodSchema(TypeSchema("const Halley::HalleyAPI&"), {}, "getAPI", true), "return doGetAPI();");
	}
	if ((int(system.access) & int(SystemAccess::World)) != 0) {
		sysClassGen.addMethodDefinition(MethodSchema(TypeSchema("Halley::World&"), {}, "getWorld", true), "return doGetWorld();");
	}
	if ((int(system.access) & int(SystemAccess::Resources)) != 0) {
		sysClassGen.addMethodDefinition(MethodSchema(TypeSchema("Halley::Resources&"), {}, "getResources", true), "return doGetAPI().core->getResources();");
	}

	// Receive messages
	bool hasReceive = false;
	Vector<String> msgsReceived;
	for (auto& msg : system.messages) {
		if (msg.send) {
			sysClassGen.addMethodDefinition(MethodSchema(TypeSchema("void"), { VariableSchema(TypeSchema("Halley::EntityId"), "entityId"), VariableSchema(TypeSchema(msg.name + "Message&", true), "msg") }, "sendMessage"), "sendMessageGeneric(entityId, msg);");
		}
		if (msg.receive) {
			hasReceive = true;
			msgsReceived.push_back(msg.name + "Message::messageIndex");
		}
	}

	// Service declarations
	for (auto& service: system.services) {
		sysClassGen.addBlankLine();
		sysClassGen.addMember(MemberSchema(service.name + "*", lowerFirst(service.name), "nullptr"));
		sysClassGen.addMethodDefinition(MethodSchema(TypeSchema(service.name + "&"), {}, "get" + service.name, true), "return *" + lowerFirst(service.name) + ";");
	}

	// Construct initBase();
	std::vector<String> initBaseMethodBody;
	for (auto& service: system.services) {
		initBaseMethodBody.push_back(lowerFirst(service.name) + " = &doGetWorld().template getService<" + service.name + ">();");
	}
	initBaseMethodBody.push_back("invokeInit<T>(static_cast<T*>(this));");
	for (auto& family: system.families) {
		initBaseMethodBody.push_back("initialiseFamilyBinding<T, " + upperFirst(family.name) + "Family>(" + family.name + "Family, static_cast<T*>(this));");
	}
	sysClassGen.addMethodDefinition(MethodSchema(TypeSchema("void"), {}, "initBase", false, false, true), initBaseMethodBody);

	auto fams = convert<FamilySchema, MemberSchema>(system.families, [](auto& fam) { return MemberSchema(TypeSchema("Halley::FamilyBinding<" + upperFirst(fam.name) + "Family>"), fam.name + "Family"); });
	auto mid = fams.begin() + std::min(fams.size(), size_t(1));
	std::vector<MemberSchema> mainFams(fams.begin(), mid);
	std::vector<MemberSchema> otherFams(mid, fams.end());
	sysClassGen
		.addBlankLine()
		.addAccessLevelSection(system.strategy == SystemStrategy::Global ? MemberAccess::Protected : MemberAccess::Private)
		.addMembers(mainFams)
		.addAccessLevelSection(MemberAccess::Protected)
		.addMembers(otherFams)
		.addBlankLine()
		.addAccessLevelSection(MemberAccess::Private)
		.addMethodDefinition(MethodSchema(TypeSchema("void"), { VariableSchema(TypeSchema(info.methodArgType), info.methodArgName) }, info.methodName + "Base", false, false, true, true), info.stratImpl)
		.addBlankLine();

	if (hasReceive) {
		Vector<String> body = { "switch (msgIndex) {" };
		for (auto& msg : system.messages) {
			if (msg.receive) {
				body.emplace_back("case " + msg.name + "Message::messageIndex: onMessagesReceived(reinterpret_cast<" + msg.name + "Message**>(msgs), idx, n); break;");
			}
		}
		body.emplace_back("}");
		sysClassGen
			.addMethodDefinition(MethodSchema(TypeSchema("void"), { VariableSchema(TypeSchema("int"), "msgIndex"), VariableSchema(TypeSchema("Halley::Message**"), "msgs"), VariableSchema(TypeSchema("size_t*"), "idx"), VariableSchema(TypeSchema("size_t"), "n") }, "onMessagesReceived", false, false, true, true), body)
			.addBlankLine()
			.addLine("template <typename M>")
			.addMethodDefinition(MethodSchema(TypeSchema("void"), { VariableSchema(TypeSchema("M**"), "msgs"), VariableSchema(TypeSchema("size_t*"), "idx"), VariableSchema(TypeSchema("size_t"), "n") }, "onMessagesReceived"), "for (size_t i = 0; i < n; i++) static_cast<T*>(this)->onMessageReceived(*msgs[i], mainFamily[idx[i]]);")
			.addBlankLine();
	}

	sysClassGen
		.addAccessLevelSection(MemberAccess::Public)
		.addCustomConstructor({}, { VariableSchema(TypeSchema(""), "System", "{" + String::concatList(convert<FamilySchema, String>(system.families, [](auto& fam) { return "&" + fam.name + "Family"; }), ", ") + "}, {" + String::concatList(msgsReceived, ", ") + "}") })
		.finish()
		.writeTo(contents);

	contents.push_back("");

	return contents;
}

Vector<String> CodegenCPP::generateSystemStub(SystemSchema& system) const
{
	auto info = SystemInfo(system);
	String systemName = system.name + "System";

	Vector<String> contents = {
		"#include <systems/" + toFileName(systemName) + ".h>",
		""
	};

	auto actualSys = CPPClassGenerator(systemName, systemName + "Base<" + systemName + ">", MemberAccess::Public, true)
		.addAccessLevelSection(MemberAccess::Public)
		.addMethodDefinition(MethodSchema(TypeSchema("void"), info.familyArgs, info.methodName, info.methodConst), "// TODO");

	for (auto& msg : system.messages) {
		if (msg.receive) {
			actualSys
				.addBlankLine()
				.addMethodDefinition(MethodSchema(TypeSchema("void"), { VariableSchema(TypeSchema(msg.name + "Message&", true), "msg"), VariableSchema(TypeSchema("MainFamily&"), "entity") }, "onMessageReceived"), "// TODO");
		}
	}

	actualSys
		.finish()
		.writeTo(contents);

	contents.insert(contents.end(), {
		"",
		"REGISTER_SYSTEM(" + systemName + ")",
		""
	});

	return contents;
}

Vector<String> CodegenCPP::generateMessageHeader(MessageSchema message)
{
	Vector<String> contents = {
		"#pragma once",
		"",
		"#include <halley.hpp>",
		""
	};

	for (auto& includeFile: message.includeFiles) {
		contents.push_back("#include \"" + includeFile + "\"");
	}
	contents.push_back("");

	auto gen = CPPClassGenerator(message.name + "Message", "Halley::Message", MemberAccess::Public, true)
		.addAccessLevelSection(MemberAccess::Public)
		.addMember(MemberSchema(TypeSchema("int", false, true, true), "messageIndex", toString(message.id)))
		.addBlankLine()
		.addMembers(message.members)
		.addBlankLine()
		.addDefaultConstructor()
		.addBlankLine();

	if (!message.members.empty()) {
		gen.addConstructor(MemberSchema::toVariableSchema(message.members))
			.addBlankLine();
	}

	gen.addMethodDefinition(MethodSchema(TypeSchema("size_t"), {}, "getSize", true, false, true, true), "return sizeof(" + message.name + "Message);")
		.finish()
		.writeTo(contents);

	return contents;
}

Path CodegenCPP::makePath(Path dir, String className, String extension) const
{
	return dir / (toFileName(className) + "." + extension).cppStr();
}

String CodegenCPP::toFileName(String className) const
{
	std::stringstream ss;

	auto isUpper = [](char c) {
		return c >= 'A' && c <= 'Z';
	};

	auto lower = [](char c) {
		return static_cast<char>(c + 32);
	};

	auto tryGet = [](const String& s, size_t i) {
		if (s.size() > i) {
			return s[i];
		} else {
			return char(0);
		}
	};

	for (size_t i = 0; i < className.size(); i++) {
		if (isUpper(className[i])) {
			if (i > 0 && !isUpper(tryGet(className, i+1))) {
				ss << '_';
			}
			ss << lower(className[i]);
		} else {
			ss << className[i];
		}
	}
	return ss.str();
}

String CodegenCPP::getComponentFileName(const ComponentSchema& component) const
{
	if (component.customImplementation) {
		return component.customImplementation.value();
	} else {
		return "components/" + toFileName(component.name + "Component") + ".h";
	}
}
