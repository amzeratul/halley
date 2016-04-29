#include <sstream>
#include "codegen_cpp.h"
#include "cpp_class_gen.h"
using namespace Halley;

static String toFileName(String className)
{
	std::stringstream ss;
	for (size_t i = 0; i < className.size(); i++) {
		if (className[i] >= 'A' && className[i] <= 'Z') {
			if (i > 0) {
				ss << '_';
			}
			ss << static_cast<char>(className[i] + 32);
		} else {
			ss << className[i];
		}
	}
	return ss.str();
}

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
	String className = component.name + "Component";

	CodeGenResult result;
	result.emplace_back(CodeGenFile("components/" + toFileName(className) + ".h", generateComponentHeader(component)));
	return result;
}

CodeGenResult CodegenCPP::generateSystem(SystemSchema system)
{
	String className = system.name + "System";

	CodeGenResult result;
	result.emplace_back(CodeGenFile("systems/" + toFileName(className) + ".h", generateSystemHeader(system)));
	return result;
}

CodeGenResult CodegenCPP::generateRegistry(const std::vector<ComponentSchema>& components, const std::vector<SystemSchema>& systems)
{
	std::vector<String> registryCpp {
		"#include <halley.hpp>",
		"using namespace Halley;",
		"",
		"// System factory functions"
	};

	for (auto& sys: systems) {
		registryCpp.push_back("System* halleyCreate" + sys.name + "System();");
	}

	registryCpp.insert(registryCpp.end(), {
		"",
		"",
		"using SystemFactoryPtr = System* (*)();",
		"using SystemFactoryMap = std::map<String, SystemFactoryPtr>;",
		"",
		"static SystemFactoryMap makeSystemFactories() {",
		"	SystemFactoryMap result;"
	});

	for (auto& sys : systems) {
		registryCpp.push_back("	result[\"" + sys.name + "System\"] = &halleyCreate" + sys.name + "System;");
	}

	registryCpp.insert(registryCpp.end(), { 
		"	return result;",
		"}",
		"",
		"namespace Halley {",
		"	std::unique_ptr<System> createSystem(String name) {",
		"		static SystemFactoryMap factories = makeSystemFactories();",
		"		return std::unique_ptr<System>(factories.at(name)());",
		"	}",
		"}"
	});

	std::vector<String> registryH{
		"#pragma once",
		"",
		"namespace Halley {",
		"	std::unique_ptr<System> createSystem(String name);",
		"}"
	};

	CodeGenResult result;
	result.emplace_back(CodeGenFile("registry.cpp", registryCpp));
	result.emplace_back(CodeGenFile("registry.h", registryH));
	return result;
}

std::vector<String> CodegenCPP::generateComponentHeader(ComponentSchema component)
{
	std::vector<String> contents = {
		"#pragma once",
		"",
		"#include <halley.hpp>",
		""
	};

	CPPClassGenerator(component.name + "Component", "Component")
		.addAccessLevelSection(CPPAccess::Public)
		.addMember(VariableSchema(TypeSchema("int", false, true, true), "componentIndex", String::integerToString(component.id)))
		.addBlankLine()
		.addMembers(component.members)
		.addBlankLine()
		.addDefaultConstructor()
		.addBlankLine()
		.addConstructor(component.members)
		.writeTo(contents);

	return contents;
}

std::vector<String> CodegenCPP::generateSystemHeader(SystemSchema system)
{
	std::vector<String> contents = {
		"#pragma once",
		"",
		"#include <halley.hpp>",
		""
	};

	// Family headers
	for (auto& fam : system.families) {
		for (auto& comp : fam.components) {
			String compName = comp.name + "Component";
			contents.emplace_back("#include \"../components/" + toFileName(compName) + ".h\"");
		}
	}

	contents.insert(contents.end(), {
		"",
		"// Generated file; do not modify."
	});

	auto sysClassGen = CPPClassGenerator(system.name + "System", "Halley::System");

	for (auto& fam : system.families) {
		auto famGen = CPPClassGenerator(upperFirst(fam.name) + "Family")
			.addAccessLevelSection(CPPAccess::Public)
			.addMember(VariableSchema(TypeSchema("Halley::EntityId", true), "entityId"))
			.addBlankLine();

		std::vector<String> compNames;
		for (auto& comp : fam.components) {
			famGen.addMember(VariableSchema(TypeSchema(comp.name + "Component* const"), lowerFirst(comp.name)));
			compNames.push_back(comp.name + "Component");
		}
		
		famGen
			.addBlankLine()
			.addTypeDefinition("Type", "Halley::FamilyType<" + String::concatList(compNames, ", ") + ">");

		sysClassGen
			.addClass(famGen)
			.addBlankLine();
	}

	std::vector<String> families;
	for (auto& fam : system.families) {
		sysClassGen.addMember(VariableSchema(TypeSchema("Halley::FamilyBinding<" + upperFirst(fam.name) + "Family>"), fam.name + "Family"));
		families.push_back("&" + fam.name + "Family");
	}

	String methodName, methodArgType, stratImpl;
	bool methodConst;
	if (system.method == SystemMethod::Update) {
		methodName = "update";
		methodArgType = "Halley::Time";
		methodConst = false;
	} else if (system.method == SystemMethod::Render) {
		methodName = "render";
		methodArgType = "Halley::Painter&";
		methodConst = true;
	} else {
		throw Exception("Unsupported method in " + system.name + "System");
	}

	std::vector<VariableSchema> familyArgs = { VariableSchema(TypeSchema(methodArgType), "p") };

	if (system.strategy == SystemStrategy::Global) {
		stratImpl = methodName + "(p);";
	} else if (system.strategy == SystemStrategy::Individual) {
		familyArgs.push_back(VariableSchema(TypeSchema("MainFamily&"), "entity"));
		stratImpl = "invokeIndividual(this, &" + system.name + "System::" + methodName + ", p, mainFamily);";
	} else {
		throw Exception("Unsupported strategy in " + system.name + "System");
	}

	sysClassGen
		.addBlankLine()
		.addMethodDefinition(MethodSchema(TypeSchema("void"), { VariableSchema(TypeSchema(methodArgType), "p") }, methodName + "Base", false, false, true), stratImpl)
		.addBlankLine()
		.addAccessLevelSection(CPPAccess::Protected)
		.addComment("Implement me:")
		.addMethodDeclaration(MethodSchema(TypeSchema("void"), familyArgs, methodName, methodConst))
		.addBlankLine()
		.addAccessLevelSection(CPPAccess::Public)
		.addCustomConstructor({}, { VariableSchema(TypeSchema(""), "System", "{" + String::concatList(families, ", ") + "}") });

	sysClassGen.writeTo(contents);
	return contents;
}
