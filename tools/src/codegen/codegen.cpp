#include "codegen.h"
#include <experimental/filesystem>
#include <yaml-cpp/yaml.h>

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "libyaml-cppmdd.lib")
#else
#pragma comment(lib, "libyaml-cppmd.lib")
#endif
#endif

using namespace Halley;

void Codegen::processFolder(String inDir, String outDir)
{
	std::cout << "Codegen \"" << inDir << "\" -> \"" << outDir << "\"." << std::endl;

	Codegen codegen;

	using namespace std::experimental::filesystem;
	path p(inDir.cppStr());
	if (exists(p) && is_directory(p)) {
		for (auto i = directory_iterator(p); i != directory_iterator(); ++i) {
			auto filePath = i->path();
			if (filePath.extension() == ".yaml") {
				codegen.addSource(filePath);
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
			std::cout << "Component: " << document["component"].as<std::string>() << std::endl;
		} else if (document["system"].IsDefined()) {
			std::cout << "System: " << document["system"].as<std::string>() << std::endl;
		} else {
			std::cout << "Warning: unknown document in stream." << std::endl;
		}
	}
}
