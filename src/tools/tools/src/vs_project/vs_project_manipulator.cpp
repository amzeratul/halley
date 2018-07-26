#include "halley/tools/vs_project/vs_project_manipulator.h"
#include "../../../engine/utils/contrib/tinyxml/ticpp.h"
#include "halley/support/logger.h"

using namespace Halley;

void VSProjectManipulator::load(const Bytes& data)
{
	origXmlData = String(reinterpret_cast<const char*>(data.data()), data.size());

	ticpp::Document doc;
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
						if (name == "ClCompile") {
							compileFiles.emplace(l2Node->GetAttribute("Include"));
						} else if (name == "ClInclude") {
							includeFiles.emplace(l2Node->GetAttribute("Include"));
						}
					}
				}
			}
		}
	}
}

Bytes VSProjectManipulator::write()
{
	return Bytes();
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
	compileFiles = files;
}

void VSProjectManipulator::setIncludeFiles(const std::set<String>& files)
{
	includeFiles = files;
}
