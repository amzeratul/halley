#pragma once

#include <halley/text/halleystring.h>
#include <halley/data_structures/vector.h>

#include "halley/data_structures/hash_map.h"

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
		World = 2,
		Resources = 4,
		MessageBridge = 8
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
		bool optional = false;
	};

	class MessageReferenceSchema
	{
	public:
		String name;
		bool send = false;
		bool receive = false;

		MessageReferenceSchema() = default;
		MessageReferenceSchema(String name, String parameter);
	};

	class FamilySchema
	{
	public:
		String name;
		Vector<ComponentReferenceSchema> components;
	};

	class ServiceSchema
	{
	public:
		String name;
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
		explicit SystemSchema(YAML::Node node, bool generate);

		String name;
		SystemStrategy strategy = SystemStrategy::Individual;
		SystemAccess access = SystemAccess::Pure;
		SystemMethod method = SystemMethod::Update;
		CodegenLanguage language = CodegenLanguage::CPlusPlus;
		int smearing = 0;
		bool generate = false;

		HashSet<String> includeFiles;

		Vector<FamilySchema> families;
		Vector<MessageReferenceSchema> messages;
		Vector<MessageReferenceSchema> systemMessages;
		Vector<ServiceSchema> services;

		bool operator< (const SystemSchema& other) const;
	};
}
