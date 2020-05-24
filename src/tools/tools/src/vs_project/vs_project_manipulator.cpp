#include "halley/tools/vs_project/vs_project_manipulator.h"
#include "../../../contrib/tinyxml/ticpp.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/path.h"

using namespace Halley;


void VSProjectManipulator::load(Path path)
{
	try {
		load(FileSystem::readFile(path));
	} catch (std::exception& e) {
		Logger::logError("Unable to read input file: " + path);
		Logger::logException(e);
		throw Exception("Failed to parse VS Project", HalleyExceptions::Tools);
	}
}

void VSProjectManipulator::load(const Bytes& data)
{
	auto origXmlData = String(reinterpret_cast<const char*>(data.data()), data.size());
	document = std::make_shared<ticpp::Document>();
	auto& doc = *document;
	doc.Parse(origXmlData);

	std::string name;
	for (ticpp::Element* l0Node = doc.FirstChildElement(); l0Node != nullptr; l0Node = l0Node->NextSiblingElement(false)) {
		l0Node->GetValue(&name);
		if (name == "Project") {
			for (ticpp::Element* l1Node = l0Node->FirstChildElement(); l1Node != nullptr; l1Node = l1Node->NextSiblingElement(false)) {
				l1Node->GetValue(&name);
				if (name == "ItemGroup") {
					for (ticpp::Element* l2Node = l1Node->FirstChildElement(); l2Node != nullptr; l2Node = l2Node->NextSiblingElement(false)) {
						l2Node->GetValue(&name);
						String fileName = l2Node->GetAttribute("Include");
						if (fileName.contains("cmake_pch")) {
							continue;
						}
						
						if (name == "ClCompile") {
							compileFiles.emplace(fileName);
						} else if (name == "ClInclude") {
							includeFiles.emplace(fileName);
						}
					}
				}
			}
		}
	}
}

Bytes VSProjectManipulator::write()
{
	TiXmlPrinter printer;
	printer.SetIndent("  ");
	document->Accept(&printer);
	String str = printer.CStr();

	str = str.replaceAll("&apos;", "'"); // lol

	Bytes result(str.size());
	memcpy(result.data(), str.c_str(), str.size());
	return result;
}

std::set<String> VSProjectManipulator::getCompileFiles()
{
	return compileFiles;
}

std::set<String> VSProjectManipulator::getIncludeFiles()
{
	return includeFiles;
}

void VSProjectManipulator::setCompileFiles(const std::set<String>& files)
{
	rewriteFiles("ClCompile", compileFiles, files);
	compileFiles = files;
}

void VSProjectManipulator::setIncludeFiles(const std::set<String>& files)
{
	rewriteFiles("ClInclude", includeFiles, files);
	includeFiles = files;
}

void VSProjectManipulator::rewriteFiles(String toolName, const std::set<String>& oldSet, const std::set<String>& newSet)
{
	std::set<String> toDelete;
	std::set<String> toAdd;

	for (auto& v: oldSet) {
		if (newSet.find(v) == newSet.end()) {
			toDelete.insert(v);
		}
	}
	for (auto& v: newSet) {
		if (oldSet.find(v) == oldSet.end()) {
			toAdd.insert(v);
		}
	}

	Logger::logInfo(toolName + ": adding " + toString(toAdd.size()) + " files, removing " + toString(toDelete.size()) + " files.");

	auto& doc = *document;

	bool addedValues = false;

	std::string name;
	for (ticpp::Element* l0Node = doc.FirstChildElement(); l0Node != nullptr; l0Node = l0Node->NextSiblingElement(false)) {
		l0Node->GetValue(&name);
		if (name == "Project") {
			for (ticpp::Element* l1Node = l0Node->FirstChildElement(); l1Node != nullptr; l1Node = l1Node->NextSiblingElement(false)) {
				l1Node->GetValue(&name);
				if (name == "ItemGroup") {
					bool compatibleItemGroup = false;
					for (ticpp::Element* l2Node = l1Node->FirstChildElement(); l2Node != nullptr;) {
						auto nextNode = l2Node->NextSiblingElement(false);
						l2Node->GetValue(&name);
						if (String(name) == toolName) {
							compatibleItemGroup = true;

							// Does it need to be removed?
							auto path = l2Node->GetAttribute("Include");
							if (toDelete.find(path) != toDelete.end()) {
								// Delete
								l1Node->RemoveChild(l2Node);
							}
						}
						l2Node = nextNode;
					}

					if (compatibleItemGroup && !addedValues) {
						addedValues = true;
						for (auto& v: toAdd) {
							ticpp::Element element(toolName);
							element.SetAttribute("Include", v.cppStr());
							l1Node->InsertEndChild(element);
						}
					}
				}
			}
		}
	}
}
