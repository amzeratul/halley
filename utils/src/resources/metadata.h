#pragma once

#include "../text/halleystring.h"
#include <memory>

namespace YAML
{
	class Node;
}

namespace Halley
{
	class ResourceDataStatic;

	class Metadata
	{
	public:
		Metadata();
		Metadata(const ResourceDataStatic& data);
		~Metadata();

		bool getBool(String key) const;
		int getInt(String key) const;
		float getFloat(String key) const;
		String getString(String key) const;

		bool getBool(String key, bool defaultValue) const;
		int getInt(String key, int defaultValue) const;
		float getFloat(String key, float defaultValue) const;
		String getString(String key, String defaultValue) const;

	private:
		std::unique_ptr<YAML::Node> root;
	};
}