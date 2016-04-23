#pragma once
#include "icode_generator.h"

namespace Halley
{
	class CodegenCPP : public ICodeGenerator
	{
	public:
		CodegenLanguage getLanguage() const override { return CodegenLanguage::CPlusPlus; }
		CodeGenResult generateComponent(ComponentSchema component) override;
		CodeGenResult generateSystem(SystemSchema system) override;
	};
}
