#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <halley/support/exception.h>
#include <halley/tools/codegen/codegen.h>
#include "cpp/codegen_cpp.h"

#ifdef _MSC_VER
	#ifdef _DEBUG
		#pragma comment(lib, "libyaml-cppmdd.lib")
	#else
		#pragma comment(lib, "libyaml-cppmd.lib")
	#endif

	#include <sys/utime.h>
#else
	#include <sys/types.h>
	#include <utime.h>
#endif


using namespace Halley;

void Codegen::run(Path inDir, Path outDir)
{
	std::cout << "Codegen \"" << inDir << "\" -> \"" << outDir << "\"." << std::endl;

	Codegen codegen;
	codegen.loadSources(inDir);
	codegen.validate();
	codegen.process();
	codegen.generateCode(outDir);
}

void Codegen::loadSources(Path p)
{
	if (exists(p) && is_directory(p)) {
		for (auto i = boost::filesystem::directory_iterator(p); i != boost::filesystem::directory_iterator(); ++i) {
			auto filePath = i->path();
			if (filePath.extension() == ".yaml") {
				addSource(filePath);
			}
		}
	}
}

void Codegen::loadSources(std::vector<Path> files)
{
	for (auto& f : files) {
		addSource(f);
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

bool Codegen::writeFile(Path filePath, const char* data, size_t dataSize, bool stub) const
{
	if (FileSystem::exists(filePath)) {
		if (stub) {
			return false;
		}

		if (file_size(filePath) == dataSize) {
			// Size matches, check if contents are identical
			std::ifstream in(filePath.string(), std::ofstream::in | std::ofstream::binary);
			Vector<char> buffer(dataSize);
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
	}

	FileSystem::createParentDir(filePath);

	// Write file
	std::ofstream out(filePath.string(), std::ofstream::out | std::ofstream::binary);
	out.write(data, dataSize);
	out.close();

	return true;
}

void Codegen::writeFiles(Path dir, const CodeGenResult& files, Stats& stats) const
{
	FileSystem::createDir(dir);
	
	for (auto& f : files) {
		Path filePath = dir / f.fileName;
		std::stringstream ss;
		for (auto& line: f.fileContents) {
			ss << line;
			ss << "\n";
		}
		auto finalData = ss.str();

		bool wrote = writeFile(filePath, &finalData[0], finalData.size(), f.stub);
		if (wrote) {
			stats.written++;
			std::cout << "* Written " << filePath << std::endl;
		} else {
			stats.skipped++;
		}
		stats.files.emplace_back(std::move(filePath));
	}
}

std::vector<Path> Codegen::generateCode(Path directory)
{
	Vector<std::unique_ptr<ICodeGenerator>> gens;
	gens.emplace_back(std::make_unique<CodegenCPP>());
	Stats stats;

	for (auto& gen : gens) {
		Path genDir = directory / gen->getDirectory();
		Vector<ComponentSchema> comps;
		Vector<SystemSchema> syss;

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

		// Registry
		writeFiles(genDir, gen->generateRegistry(comps, syss), stats);
	}

	// Has changes
	if (stats.written > 0) {
		using namespace boost::filesystem;
		auto cmakeLists = directory.parent_path() / path("CMakeLists.txt");
		std::cout << "Touching " << cmakeLists.string() << std::endl;
		utime(cmakeLists.string().c_str(), nullptr);
	}

	std::cout << "Codegen: " << stats.written << " written, " << stats.skipped << " skipped." << std::endl;
	return stats.files;
}

void Codegen::addSource(Path path)
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
