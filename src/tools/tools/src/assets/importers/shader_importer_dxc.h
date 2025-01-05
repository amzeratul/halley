#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	enum class ShaderType;
    class MaterialDefinition;

	class ShaderImporterDXC
	{
	public:
        static Bytes compileDXIL(const String& name, ShaderType type, const Bytes& data, const String& language, const MaterialDefinition& material);
	};
}
