#pragma once

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

	class FamilySchema
	{
	public:
		String name;
		std::vector<ComponentReferenceSchema> components;
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
		int smearing = 0;
		std::vector<FamilySchema> families;
	};
}
