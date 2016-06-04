#pragma once

#include <halley/text/halleystring.h>
#include <halley/data_structures/vector.h>

namespace YAML
{
	class Node;
}

namespace Halley
{
	enum class SystemStrategy
	{
		Global,
		Individual,
		Parallel
	};

	enum class SystemAccess
	{
		Pure = 0,
		API = 1,
		World = 2
	};

	enum class SystemMethod
	{
		Update,
		Render
	};

	class ComponentReferenceSchema
	{
	public:
		String name;
		bool write = false;
	};

	class MessageReferenceSchema
	{
	public:
		String name;
		bool send = false;
		bool receive = false;
	};

	class FamilySchema
	{
	public:
		String name;
		Vector<ComponentReferenceSchema> components;
	};

	enum class CodegenLanguage
	{
		CPlusPlus,
		Lua
	};

	class SystemSchema
	{
	public:
		SystemSchema();
		explicit SystemSchema(YAML::Node node);

		String name;
		SystemStrategy strategy = SystemStrategy::Individual;
		SystemAccess access = SystemAccess::Pure;
		SystemMethod method = SystemMethod::Update;
		CodegenLanguage language = CodegenLanguage::CPlusPlus;
		int smearing = 0;

		Vector<FamilySchema> families;
		Vector<MessageReferenceSchema> messages;
	};
}
