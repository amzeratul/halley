#pragma once
#include "halley/utils/utils.h"
#include "halley/text/halleystring.h"
#include <set>

namespace Halley {
	class VSProjectManipulator {
	public:
		void load(const Bytes& data);
		Bytes write();

		std::set<String> getCompileFiles();
		std::set<String> getIncludeFiles();

		void setCompileFiles(const std::set<String>& files);
		void setIncludeFiles(const std::set<String>& files);

	private:
		String origXmlData;
		std::set<String> includeFiles;
		std::set<String> compileFiles;
	};
}
