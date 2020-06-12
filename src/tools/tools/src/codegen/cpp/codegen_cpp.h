#pragma once
#include <halley/tools/codegen/icode_generator.h>
#include "halley/file/path.h"

namespace Halley
{
	class CodegenCPP final : public ICodeGenerator
	{
	public:
		Path getDirectory() const override { return "cpp"; }
		CodegenLanguage getLanguage() const override { return CodegenLanguage::CPlusPlus; }

		CodeGenResult generateComponent(ComponentSchema component) override;
		CodeGenResult generateSystem(SystemSchema system, const HashMap<String, ComponentSchema>& components) override;
		CodeGenResult generateMessage(MessageSchema message) override;
		CodeGenResult generateSystemMessage(SystemMessageSchema message) override;

		CodeGenResult generateRegistry(const Vector<ComponentSchema>& components, const Vector<SystemSchema>& systems) override;

	private:
		Vector<String> generateComponentHeader(ComponentSchema component);
		Vector<String> generateSystemHeader(SystemSchema& system, const HashMap<String, ComponentSchema>& components) const;
		Vector<String> generateSystemStub(SystemSchema& system) const;
		Vector<String> generateMessageHeader(const MessageSchema& message, const String& suffix);

		Path makePath(Path dir, String className, String extension) const;
		String toFileName(String className) const;
		String getComponentFileName(const ComponentSchema& component) const;
	};
}
