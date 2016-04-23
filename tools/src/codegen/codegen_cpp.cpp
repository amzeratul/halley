#include <sstream>
#include "codegen_cpp.h"
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
	String className = component.name + "Component";
	std::vector<String> contents = {
		"#pragma once",
		"",
		"#include <halley.hpp>",
		"",
		"class " + className + " : public Component {",
		"public:",
		"	constexpr static int componentIndex = " + String::integerToString(component.id) + ";",
		"",
	};
	for (auto& member : component.members) {
		contents.emplace_back(String("\t") + (member.isConst ? "const " : "") + member.type + " " + member.name + ";");
	}
	contents.emplace_back("};");
	return contents;
}

std::vector<String> CodegenCPP::generateSystemHeader(SystemSchema system)
{
	String className = system.name + "System";
	std::vector<String> contents = {
		"#pragma once",
		"",
		"#include <halley.hpp>"
	};

	// Family headers
	for (auto& fam: system.families) {
		for (auto& comp: fam.components) {
			String compName = comp.name + "Component";
			contents.emplace_back("#include \"../components/" + toFileName(compName) + ".h\"");
		}
	}

	contents.insert(contents.end(), {
		"",
		"// Generated file; do not modify.",
		"class " + className + " : public Halley::System {"
	});

	// Family declarations
	String familyList;
	for (auto& fam: system.families) {
		if (familyList.size() != 0) {
			familyList += ", ";
		}
		familyList += "&" + fam.name + "Family";

		contents.insert(contents.end(), {
			"	class " + upperFirst(fam.name) + "Family {",
			"	public:",
			"		const Halley::EntityId entityId;",
			"		"
		});

		String compsList;
		for (auto& comp: fam.components) {
			if (compsList.size() != 0) {
				compsList += ", ";
			}
			compsList += comp.name + "Component";
			contents.emplace_back("		" + comp.name + "Component* const " + lowerFirst(comp.name) + ";");
		}

		contents.insert(contents.end(), {
			"		",
			"		using Type = Halley::FamilyType<" + compsList + ">;",
			"	};",
			"	"
		});
	}

	// Family bindings
	for (auto& fam : system.families) {
		contents.emplace_back("	Halley::FamilyBinding<" + upperFirst(fam.name) + "Family> " + fam.name + "Family;");
	}
	
	String methodName, methodArgType, familyArg, methodConst, stratImpl;
	if (system.method == SystemMethod::Update) {
		methodName = "update";
		methodArgType = "Halley::Time";
		methodConst = "";
	} else if (system.method == SystemMethod::Render) {
		methodName = "render";
		methodArgType = "Halley::Painter&";
		methodConst = " const";
	} else {
		throw Exception("Unsupported method in " + system.name + "System");
	}

	if (system.strategy == SystemStrategy::Global) {
		familyArg = "";
		stratImpl = methodName + "(p);";
	} else if (system.strategy == SystemStrategy::Individual) {
		familyArg = ", MainFamily& entity";
		stratImpl = "invokeIndividual(this, &" + system.name + "System::" + methodName + ", p, mainFamily);";
	} else {
		throw Exception("Unsupported strategy in " + system.name + "System");
	}

	contents.insert(contents.end(), {
		"",
		"	void " + methodName + "Base(" + methodArgType + " p) override { " + stratImpl + " };",
		"",
		"protected:",
		"	void " + methodName + "(" + methodArgType + " p" + familyArg + ")" + methodConst + "; // Implement me",
		"",
		"public:",
		"	" + system.name + "System() : System({" + familyList + "}) {}",
		"};"
	});

	return contents;
}
