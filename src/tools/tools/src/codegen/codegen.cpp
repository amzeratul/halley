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

int Codegen::getHeaderVersion(gsl::span<const char> data, size_t& endOfHeader)
{
	const String str(data.data(), std::min(data.size(), size_t(128)));
	const auto endLine = str.find('\n');
	if (endLine == std::string::npos) {
		endOfHeader = 0;
		return 0;
	}

	endOfHeader = endLine;
	
	const String firstLine = str.left(endLine).trimBoth();
	if (firstLine.startsWith("// Halley codegen version ")) {
		return firstLine.mid(26).toInteger();
	} else {
		return 0;
	}
}

bool Codegen::writeFile(const Path& filePath, gsl::span<const char> data, bool stub)
{
	if (FileSystem::exists(filePath)) {
		if (stub) {
			return false;
		}

		// Read existing file
		std::ifstream in(filePath.string(), std::ofstream::in | std::ofstream::binary);
		Vector<char> buffer(data.size());
		in.read(&buffer[0], data.size());
		in.close();

		// End of headers
		size_t oldHeaderEnd = 0;
		size_t newHeaderEnd = 0;

		// Check if existing file is more recent
		if (getHeaderVersion(buffer, oldHeaderEnd) > currentCodegenVersion) {
			throw Exception("Codegen is out of date, please build editor.", HalleyExceptions::Tools);
		}

		// Check if contents are identical
		getHeaderVersion(data, newHeaderEnd);
		if (std::equal(buffer.begin() + oldHeaderEnd, buffer.end(), data.begin() + newHeaderEnd, data.end())) {
			return false;
		}
	} else {
		FileSystem::createParentDir(filePath);
	}

	// Write file
	std::ofstream out(filePath.string(), std::ofstream::out | std::ofstream::binary);
	out.write(data.data(), data.size());
	out.close();

	return true;
}

void Codegen::writeFiles(const Path& dir, const CodeGenResult& files, Stats& stats)
{
	FileSystem::createDir(dir);
	
	constexpr const char* lineBreak = getPlatform() == GamePlatform::Windows ? "\r\n" : "\n";
	
	for (auto& f : files) {
		Path filePath = dir / f.fileName;
		std::stringstream ss;
		ss << "// Halley codegen version " << currentCodegenVersion << lineBreak;
		for (auto& line: f.fileContents) {
			ss << line << lineBreak;
		}
		auto finalData = ss.str();

		const bool wrote = writeFile(filePath, gsl::span<const char>(&finalData[0], finalData.size()), f.stub);
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

Vector<Path> Codegen::generateCode(const ECSData& data, Path directory)
{
	auto components = data.getComponents();
	auto systems = data.getSystems();
	auto messages = data.getMessages();
	auto systemMessages = data.getSystemMessages();
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
		Vector<MessageSchema> msgs;
		Vector<SystemMessageSchema> sysMsgs;

		for (auto& comp: components) {
			if (comp.second.generate) {
				writeFiles(genDir, gen->generateComponent(comp.second), stats);
			}
			comps.push_back(comp.second);
		}
		for (auto& sys: systems) {
			if (sys.second.generate && sys.second.language == gen->getLanguage()) {
				writeFiles(genDir, gen->generateSystem(sys.second, components, systemMessages), stats);
			}
			syss.push_back(sys.second);
		}
		for (auto& msg: messages) {
			if (msg.second.generate) {
				writeFiles(genDir, gen->generateMessage(msg.second), stats);
			}
			msgs.push_back(msg.second);
		}
		for (auto& sysMsg: systemMessages) {
			if (sysMsg.second.generate) {
				writeFiles(genDir, gen->generateSystemMessage(sysMsg.second), stats);
			}
			sysMsgs.push_back(sysMsg.second);
		}

		// Registry
		writeFiles(genDir, gen->generateRegistry(comps, syss, msgs, sysMsgs), stats);
	}

	// Has changes
	if (stats.written > 0) {
		auto cmakeLists = directory.parentPath() / Path("CMakeLists.txt");
		utime(cmakeLists.string().c_str(), nullptr);
	}

	Vector<Path> out;
	for (auto& f : stats.files) {
		out.push_back(FileSystem::getRelative(f, directory));
	}
	return out;
}
