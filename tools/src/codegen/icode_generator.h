#pragma once

#include "component_schema.h"
#include "system_schema.h"

namespace Halley
{
	struct CodeGenFile
	{
		String fileName;
		std::vector<String> fileContents;
	};

	using CodeGenResult = std::vector<CodeGenFile>;

	class ICodeGenerator
	{
	public:
		virtual ~ICodeGenerator() {}

		virtual CodegenLanguage getLanguage() const = 0;
		virtual CodeGenResult generateComponent(ComponentSchema component) = 0;
		virtual CodeGenResult generateSystem(SystemSchema system) = 0;
	};
}
