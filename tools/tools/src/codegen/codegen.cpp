#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <halley/support/exception.h>
#include <halley/tools/codegen/codegen.h>
#include "cpp/codegen_cpp.h"
#include "halley/tools/file/filesystem.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

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
	throw Exception("Not supported");
}

Codegen::Codegen(bool verbose)
	: verbose(verbose)
{
}

void Codegen::loadSources(std::vector<std::pair<String, gsl::span<const gsl::byte>>> files, ProgressReporter progress)
{
	int i = 0;
	for (auto& f : files) {
		addSource(f.first, f.second);
		if (!progress(float(i) / float(files.size()), f.first)) {
			return;
		}
	}
}

void Codegen::validate(ProgressReporter progress)
{
	int i = 0;
	for (auto& sys : systems) {
		if (!progress(float(i) / float(systems.size()), sys.first)) {
			return;
		}

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

		for (auto& msg: sys.second.messages) {
			if (messages.find(msg.name) == messages.end()) {
				throw Exception("Unknown message \"" + msg.name + "\" in system \"" + sys.second.name + "\".");
			}
		}

		for (auto& service: sys.second.services) {
			if (types.find(service.name) == types.end()) {
				throw Exception("Unknown service \"" + service.name + "\" in system \"" + sys.second.name + "\".");
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

			for (auto& m: comp.second.members) {
				String i = getInclude(m.type.name);
				if (i != "") {
					comp.second.includeFiles.insert(i);
				}
			}
		}
	}
	
	{
		int id = 0;
		for (auto& msg : messages) {
			msg.second.id = id++;

			for (auto& m: msg.second.members) {
				String i = getInclude(m.type.name);
				if (i != "") {
					msg.second.includeFiles.insert(i);
				}
			}
		}
	}

	for (auto& system: systems) {
		auto& sys = system.second;

		for (auto& fam: sys.families) {
			// Sorting the components ensures that different systems which use the same family will not corrupt memory by accessing them in different orders
			std::sort(fam.components.begin(), fam.components.end(), [] (const ComponentReferenceSchema& a, const ComponentReferenceSchema& b) -> bool {
				return a.name < b.name;
			});
		}

		for (auto& service: sys.services) {
			sys.includeFiles.insert(getInclude(service.name));
		}
	}
}

bool Codegen::writeFile(Path filePath, const char* data, size_t dataSize, bool stub) const
{
	if (FileSystem::exists(filePath)) {
		if (stub) {
			return false;
		}

		if (FileSystem::fileSize(filePath) == dataSize) {
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
			if (verbose) {
				std::cout << "* Written " << filePath << std::endl;
			}
		} else {
			stats.skipped++;
		}
		if (!f.stub) {
			stats.files.emplace_back(std::move(filePath));
		}
	}
}

std::vector<Path> Codegen::generateCode(Path directory, ProgressReporter progress)
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
		auto cmakeLists = directory.parentPath() / Path("CMakeLists.txt");
		if (verbose) {
			std::cout << "Touching " << cmakeLists.string() << std::endl;
		}
		utime(cmakeLists.string().c_str(), nullptr);
	}

	if (verbose) {
		std::cout << "Codegen: " << stats.written << " written, " << stats.skipped << " skipped." << std::endl;
	}

	std::vector<Path> out;
	for (auto& f : stats.files) {
		out.push_back(FileSystem::getRelative(f, directory));
	}
	return out;
}

void Codegen::addSource(String path, gsl::span<const gsl::byte> data)
{
	String strData(reinterpret_cast<const char*>(data.data()), data.size());
	auto documents = YAML::LoadAll(strData.cppStr());

	for (auto document: documents) {
		String curPos = path + ":" + toString(document.Mark().line) + ":" + toString(document.Mark().column);

		if (!document.IsDefined() || document.IsNull()) {
			throw Exception("Invalid document in stream.");
		}
			
		if (document.IsScalar()) {
			throw Exception("YAML parse error in codegen definitions:\n\"" + document.as<std::string>() + "\"\nat " + curPos);
		} else if (document["component"].IsDefined()) {
			addComponent(document);
		} else if (document["system"].IsDefined()) {
			addSystem(document);
		} else if (document["message"].IsDefined()) {
			addMessage(document);
		} else if (document["type"].IsDefined()) {
			addType(document);
		} else {
			throw Exception("YAML parse error in codegen definitions: unknown type\nat " + curPos);
		}
	}
}

void Codegen::addComponent(YAML::Node rootNode)
{
	auto comp = ComponentSchema(rootNode["component"]);

	if (components.find(comp.name) == components.end()) {
		components[comp.name] = comp;
	} else {
		throw Exception("Component already declared: " + comp.name);
	}
}

void Codegen::addSystem(YAML::Node rootNode)
{
	auto sys = SystemSchema(rootNode["system"]);

	if (systems.find(sys.name) == systems.end()) {
		systems[sys.name] = sys;
	} else {
		throw Exception("System already declared: " + sys.name);
	}
}

void Codegen::addMessage(YAML::Node rootNode)
{
	auto msg = MessageSchema(rootNode["message"]);

	if (messages.find(msg.name) == messages.end()) {
		messages[msg.name] = msg;
	} else {
		throw Exception("Message already declared: " + msg.name);
	}
}

void Codegen::addType(YAML::Node rootNode)
{
	auto t = CustomTypeSchema(rootNode["type"]);

	if (types.find(t.name) == types.end()) {
		types[t.name] = t;
	} else {
		throw Exception("Type already declared: " + t.name);
	}
}

String Codegen::getInclude(String typeName) const
{
	auto i = types.find(typeName);
	if (i != types.end()) {
		return i->second.includeFile;
	}
	return "";
}
