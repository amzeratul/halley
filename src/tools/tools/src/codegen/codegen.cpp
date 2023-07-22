#include <fstream>
#include <sstream>
#include <iostream>
#include <halley/support/exception.h>
#include <halley/tools/codegen/codegen.h>
#include "cpp/codegen_cpp.h"
#include "halley/tools/file/filesystem.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"
#include "halley/game/game_platform.h"
#include "halley/plugin/iasset_importer.h"
#include "halley/tools/ecs/ecs_data.h"

#ifdef _MSC_VER
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

bool Codegen::needsToWriteFile(const Path& filePath, gsl::span<const char> data, bool stub)
{
	if (!FileSystem::exists(filePath)) {
		return true;
	}

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
	return !std::equal(buffer.begin() + oldHeaderEnd, buffer.end(), data.begin() + newHeaderEnd, data.end());
}

void Codegen::writeFiles(const Path& outputDir, const Path& prefix, const CodeGenResult& files, Stats& stats, IAssetCollector* collector)
{
	constexpr const char* lineBreak = getPlatform() == GamePlatform::Windows ? "\r\n" : "\n";
	
	for (auto& f: files) {
		Path filePath = outputDir / prefix / f.fileName;
		std::stringstream ss;
		ss << "// Halley codegen version " << currentCodegenVersion << lineBreak;
		for (auto& line: f.fileContents) {
			ss << line << lineBreak;
		}
		auto finalData = ss.str();

		const auto data = gsl::span<const char>(finalData.data(), finalData.size());
		if (needsToWriteFile(filePath, data, f.stub)) {
			const auto data2 = gsl::span<const gsl::byte>(reinterpret_cast<const gsl::byte*>(data.data()), data.size());
			collector->output(prefix / f.fileName, data2);
			stats.written++;
		} else {
			stats.skipped++;
		}
		if (!f.stub) {
			stats.files.emplace_back(std::move(filePath));
		}
	}
}

Vector<Path> Codegen::generateCode(const ECSData& data, Path directory, IAssetCollector* collector)
{
	auto components = data.getComponents();
	auto systems = data.getSystems();
	auto messages = data.getMessages();
	auto systemMessages = data.getSystemMessages();

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
		const auto prefix = gen->getDirectory();
		Vector<ComponentSchema> comps;
		Vector<SystemSchema> syss;
		Vector<MessageSchema> msgs;
		Vector<SystemMessageSchema> sysMsgs;

		for (auto& comp: components) {
			if (comp.second.generate) {
				writeFiles(directory, prefix, gen->generateComponent(comp.second), stats, collector);
			}
			comps.push_back(comp.second);
		}
		for (auto& sys: systems) {
			if (sys.second.generate && sys.second.language == gen->getLanguage()) {
				writeFiles(directory, prefix, gen->generateSystem(sys.second, components, messages, systemMessages), stats, collector);
			}
			syss.push_back(sys.second);
		}
		for (auto& msg: messages) {
			if (msg.second.generate) {
				writeFiles(directory, prefix, gen->generateMessage(msg.second), stats, collector);
			}
			msgs.push_back(msg.second);
		}
		for (auto& sysMsg: systemMessages) {
			if (sysMsg.second.generate) {
				writeFiles(directory, prefix, gen->generateSystemMessage(sysMsg.second), stats, collector);
			}
			sysMsgs.push_back(sysMsg.second);
		}

		// Registry
		writeFiles(directory, prefix, gen->generateRegistry(comps, syss, msgs, sysMsgs), stats, collector);
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
