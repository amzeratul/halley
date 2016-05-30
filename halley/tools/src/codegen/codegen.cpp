#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <halley/support/exception.h>
#include "halley/tools/codegen/codegen.h"
#include "component_schema.h"
#include "system_schema.h"
#include "message_schema.h"
#include "icode_generator.h"
#include "cpp/codegen_cpp.h"

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
	using namespace boost::filesystem;
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
		bool hasMain = false;
		std::set<String> famNames;

		for (auto& fam : sys.second.families) {
			if (fam.name == "main") {
				hasMain = true;
			}
			if (famNames.find(fam.name) != famNames.end()) {
				throw Exception("System " + sys.second.name + " already has a family named " + fam.name);
			}
			famNames.emplace(fam.name);

			for (auto& comp : fam.components) {
				if (components.find(comp.name) == components.end()) {
					throw Exception("Unknown component \"" + comp.name + "\" in family \"" + fam.name + "\" of system \"" + sys.second.name + "\".");
				}
			}
		}

		if (sys.second.strategy == SystemStrategy::Individual || sys.second.strategy == SystemStrategy::Parallel) {
			if (!hasMain) {
				throw Exception("System " + sys.second.name + " needs to have a main family due to its strategy.");
			}
			if (sys.second.strategy == SystemStrategy::Parallel) {
				if (sys.second.families.size() != 1) {
					throw Exception("System " + sys.second.name + " can only have one family due to its strategy.");
				}
			}
		}
	}
}

void Codegen::process()
{
	{
		int id = 0;
		for (auto& comp : components) {
			comp.second.id = id++;
		}
	}{
		int id = 0;
		for (auto& msg : messages) {
			msg.second.id = id++;
		}
	}
}

bool Codegen::writeFile(String dstPath, const char* data, size_t dataSize) const
{
	using namespace boost::filesystem;
	path filePath(dstPath.cppStr());
	if (exists(filePath) && file_size(filePath) == dataSize) {
		// Size matches, check if contents are identical
		std::ifstream in(dstPath, std::ofstream::in | std::ofstream::binary);
		std::vector<char> buffer(dataSize);
		in.read(&buffer[0], dataSize);
		in.close();

		bool identical = true;
		for (size_t i = 0; i < dataSize; i++) {
			if (buffer[i] != data[i]) {
				identical = false;
				break;
			}
		}

		if (identical) {
			return false;
		}
	}

	// Ensure directory existance
	path dir = filePath.parent_path();
	if (!exists(dir)) {
		create_directories(dir);
	}

	// Write file
	std::ofstream out(dstPath, std::ofstream::out | std::ofstream::binary);
	out.write(data, dataSize);
	out.close();

	return true;
}

void Codegen::writeFiles(String directory, const CodeGenResult& files, Stats& stats) const
{
	using namespace boost::filesystem;

	path dir(directory.cppStr());
	if (!exists(dir)) {
		create_directories(dir);
		std::cout << "Created directory " << dir << std::endl;
	}
	
	for (auto& f : files) {
		path filePath = dir;
		filePath.append(f.fileName.cppStr());
		std::stringstream ss;
		for (auto& line: f.fileContents) {
			ss << line;
			ss << "\n";
		}
		auto finalData = ss.str();

		bool wrote = writeFile(filePath.string(), &finalData[0], finalData.size());
		if (wrote) {
			stats.written++;
			std::cout << "* Written " << filePath << std::endl;
		} else {
			stats.skipped++;
		}
	}
}

void Codegen::generateCode(String directory)
{
	std::vector<std::unique_ptr<ICodeGenerator>> gens;
	gens.emplace_back(std::make_unique<CodegenCPP>());
	Stats stats;

	for (auto& gen : gens) {
		String genDir = directory + "/" + gen->getDirectory();
		std::vector<ComponentSchema> comps;
		std::vector<SystemSchema> syss;

		for (auto& comp : components) {
			writeFiles(genDir, gen->generateComponent(comp.second), stats);
			comps.push_back(comp.second);
		}
		for (auto& sys : systems) {
			if (sys.second.language == gen->getLanguage()) {
				writeFiles(genDir, gen->generateSystem(sys.second), stats);
			}
			syss.push_back(sys.second);
		}
		for (auto& msg : messages) {
			writeFiles(genDir, gen->generateMessage(msg.second), stats);
		}

		writeFiles(genDir, gen->generateRegistry(comps, syss), stats);
	}

	std::cout << "Codegen: " << stats.written << " written, " << stats.skipped << " skipped." << std::endl;
}

void Codegen::addSource(boost::filesystem::path path)
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
		} else if (document["message"].IsDefined()) {
			addMessage(document);
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

void Codegen::addMessage(YAML::Node rootNode)
{
	String name = rootNode["message"].as<std::string>();
	if (messages.find(name) == messages.end()) {
		messages[name] = MessageSchema(rootNode);
	} else {
		throw Exception("Message already declared: " + name);
	}
}
