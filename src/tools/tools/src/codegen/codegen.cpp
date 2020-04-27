#include "../yaml/halley-yamlcpp.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <halley/support/exception.h>
#include <halley/tools/codegen/codegen.h>
#include "cpp/codegen_cpp.h"
#include "halley/tools/file/filesystem.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"
#include "halley/core/game/game_platform.h"
#include "halley/tools/ecs/ecs_data.h"

#ifdef _MSC_VER
	#ifndef USE_VCPKG_YAML
		#ifdef _DEBUG
			#pragma comment(lib, "yaml-cppd.lib")
		#else
			#pragma comment(lib, "yaml-cpp.lib")
		#endif
	#endif // !USE_VCPKG_YAML


	#include <sys/utime.h>
#else
	#include <sys/types.h>
	#include <utime.h>
#endif


using namespace Halley;

void Codegen::run(Path inDir, Path outDir)
{
	throw Exception("Not supported", HalleyExceptions::Tools);
}

bool Codegen::writeFile(Path filePath, const char* data, size_t dataSize, bool stub)
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

void Codegen::writeFiles(Path dir, const CodeGenResult& files, Stats& stats)
{
	FileSystem::createDir(dir);
	
	for (auto& f : files) {
		Path filePath = dir / f.fileName;
		std::stringstream ss;
		for (auto& line: f.fileContents) {
			ss << line;
			if constexpr (getPlatform() == GamePlatform::Windows) {
				ss << "\r\n";
			} else {
				ss << "\n";
			}
		}
		auto finalData = ss.str();

		bool wrote = writeFile(filePath, &finalData[0], finalData.size(), f.stub);
		if (wrote) {
			stats.written++;
		} else {
			stats.skipped++;
		}
		if (!f.stub) {
			stats.files.emplace_back(std::move(filePath));
		}
	}
}

std::vector<Path> Codegen::generateCode(const ECSData& data, Path directory)
{
	auto components = data.getComponents();
	auto systems = data.getSystems();
	auto messages = data.getMessages();
	auto types = data.getCustomTypes();

	for (auto& system : systems) {
		auto& sys = system.second;

		for (auto& fam : sys.families) {
			// Sorting the components ensures that different systems which use the same family will not corrupt memory by accessing them in different orders
			std::sort(fam.components.begin(), fam.components.end(), [](const ComponentReferenceSchema& a, const ComponentReferenceSchema& b) -> bool {
				return a.name < b.name;
			});
		}
	}

	Vector<std::unique_ptr<ICodeGenerator>> gens;
	gens.emplace_back(std::make_unique<CodegenCPP>());
	Stats stats;

	for (auto& gen : gens) {
		Path genDir = directory / gen->getDirectory();
		Vector<ComponentSchema> comps;
		Vector<SystemSchema> syss;

		for (auto& comp : components) {
			if (comp.second.generate) {
				writeFiles(genDir, gen->generateComponent(comp.second), stats);
			}
			comps.push_back(comp.second);
		}
		for (auto& sys : systems) {
			if (sys.second.generate && sys.second.language == gen->getLanguage()) {
				writeFiles(genDir, gen->generateSystem(sys.second, components), stats);
			}
			syss.push_back(sys.second);
		}
		for (auto& msg : messages) {
			if (msg.second.generate) {
				writeFiles(genDir, gen->generateMessage(msg.second), stats);
			}
		}

		// Registry
		writeFiles(genDir, gen->generateRegistry(comps, syss), stats);
	}

	// Has changes
	if (stats.written > 0) {
		auto cmakeLists = directory.parentPath() / Path("CMakeLists.txt");
		utime(cmakeLists.string().c_str(), nullptr);
	}

	std::vector<Path> out;
	for (auto& f : stats.files) {
		out.push_back(FileSystem::getRelative(f, directory));
	}
	return out;
}
