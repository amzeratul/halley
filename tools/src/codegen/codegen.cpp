#include <experimental/filesystem>
#include <yaml-cpp/yaml.h>
#include "codegen.h"
#include "component_schema.h"
#include "system_schema.h"
#include "icode_generator.h"
#include "codegen_cpp.h"

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "libyaml-cppmdd.lib")
#else
#pragma comment(lib, "libyaml-cppmd.lib")
#endif
#endif

using namespace Halley;

void Codegen::run(String inDir, String outDir)
{
	std::cout << "Codegen \"" << inDir << "\" -> \"" << outDir << "\"." << std::endl;

	Codegen codegen;
	codegen.loadSources(inDir);
	codegen.validate();
	codegen.process();
	codegen.generateCode(outDir);
}

void Codegen::loadSources(String directory)
{
	using namespace std::experimental::filesystem;
	path p(directory.cppStr());
	if (exists(p) && is_directory(p)) {
		for (auto i = directory_iterator(p); i != directory_iterator(); ++i) {
			auto filePath = i->path();
			if (filePath.extension() == ".yaml") {
				addSource(filePath);
			}
		}
	}
}

void Codegen::validate()
{
	for (auto& sys : systems) {
		for (auto& fam : sys.second.families) {
			for (auto& comp : fam.components) {
				if (components.find(comp.name) == components.end()) {
					throw Exception("Unknown component \"" + comp.name + "\" in family \"" + fam.name + "\" of system \"" + sys.second.name + "\".");
				}
			}
		}
	}
}

void Codegen::process()
{
	int id = 0;
	for (auto& comp: components) {
		comp.second.id = id++;
	}
}

void Codegen::writeFiles(String directory, const CodeGenResult& files)
{
	// TODO
}

void Codegen::generateCode(String directory)
{
	std::vector<std::unique_ptr<ICodeGenerator>> gens;
	gens.emplace_back(std::make_unique<CodegenCPP>());

	for (auto& gen : gens) {
		for (auto& comp : components) {
			writeFiles(directory, gen->generateComponent(comp.second));
		}
		for (auto& sys : systems) {
			if (sys.second.language == gen->getLanguage()) {
				writeFiles(directory, gen->generateSystem(sys.second));
			}
		}
	}
}

void Codegen::addSource(std::experimental::filesystem::path path)
{
	std::vector<YAML::Node> documents;
	
	try {
		documents = YAML::LoadAllFromFile(path.string());
	} catch (const std::exception& e) {
		std::cout << "Failed to load " << path.string() << ": " << e.what() << std::endl;
	}

	for (auto document: documents) {
		if (document["component"].IsDefined()) {
			addComponent(document);
		} else if (document["system"].IsDefined()) {
			addSystem(document);
		} else {
			std::cout << "Warning: unknown document in stream." << std::endl;
		}
	}
}

void Codegen::addComponent(YAML::Node rootNode)
{
	String name = rootNode["component"].as<std::string>();
	if (components.find(name) == components.end()) {
		components[name] = ComponentSchema(rootNode);
	} else {
		throw Exception("Component already declared: " + name);
	}
}

void Codegen::addSystem(YAML::Node rootNode)
{
	String name = rootNode["system"].as<std::string>();
	if (systems.find(name) == systems.end()) {
		systems[name] = SystemSchema(rootNode);
	} else {
		throw Exception("System already declared: " + name);
	}
}
