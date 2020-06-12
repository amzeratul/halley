#pragma once

#include "halley/tools/ecs/component_schema.h"
#include "halley/tools/ecs/system_schema.h"
#include "halley/tools/ecs/message_schema.h"
#include "halley/tools/ecs/custom_type_schema.h"
#include "halley/file/path.h"
#include "halley/data_structures/hash_map.h"

namespace Halley
{
	class SystemMessageSchema;

	struct CodeGenFile
	{
		Path fileName;
		Vector<String> fileContents;
		bool stub = false;

		CodeGenFile() {}

		CodeGenFile(Path fileName, Vector<String> contents, bool stub = false)
			: fileName(fileName)
			, fileContents(contents)
			, stub(stub)
		{}
	};

	using CodeGenResult = Vector<CodeGenFile>;

	class ICodeGenerator
	{
	public:
		virtual ~ICodeGenerator() {}

		virtual CodegenLanguage getLanguage() const = 0;
		virtual Path getDirectory() const = 0;

		virtual CodeGenResult generateComponent(ComponentSchema component) = 0;
		virtual CodeGenResult generateSystem(SystemSchema system, const HashMap<String, ComponentSchema>& components) = 0;
		virtual CodeGenResult generateMessage(MessageSchema message) = 0;
		virtual CodeGenResult generateSystemMessage(SystemMessageSchema message) = 0;

		virtual CodeGenResult generateRegistry(const Vector<ComponentSchema>& components, const Vector<SystemSchema>& systems) = 0;
	};
}
