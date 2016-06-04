#pragma once

#include "component_schema.h"
#include "system_schema.h"
#include "message_schema.h"

namespace Halley
{
	struct CodeGenFile
	{
		String fileName;
		Vector<String> fileContents;

		CodeGenFile() {}

		CodeGenFile(String fileName, Vector<String> contents)
			: fileName(fileName)
			, fileContents(contents)
		{}
	};

	using CodeGenResult = Vector<CodeGenFile>;

	class ICodeGenerator
	{
	public:
		virtual ~ICodeGenerator() {}

		virtual CodegenLanguage getLanguage() const = 0;
		virtual String getDirectory() const = 0;

		virtual CodeGenResult generateComponent(ComponentSchema component) = 0;
		virtual CodeGenResult generateSystem(SystemSchema system) = 0;
		virtual CodeGenResult generateMessage(MessageSchema message) = 0;

		virtual CodeGenResult generateRegistry(const Vector<ComponentSchema>& components, const Vector<SystemSchema>& systems) = 0;
	};
}
