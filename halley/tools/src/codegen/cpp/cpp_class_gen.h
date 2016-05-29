#pragma once

#include "../fields_schema.h"

namespace Halley
{
	enum class CPPAccess
	{
		Public,
		Protected,
		Private
	};

	class CPPClassGenerator
	{
	public:
		explicit CPPClassGenerator(String name);
		explicit CPPClassGenerator(String name, String baseClass, CPPAccess inheritanceType = CPPAccess::Public, bool isFinal = false);

		CPPClassGenerator& addBlankLine();
		CPPClassGenerator& addLine(String line);

		CPPClassGenerator& addClass(CPPClassGenerator& otherClass);
		CPPClassGenerator& addComment(String comment);
		CPPClassGenerator& addAccessLevelSection(CPPAccess access);
		CPPClassGenerator& addMember(VariableSchema member);
		CPPClassGenerator& addMembers(const std::vector<VariableSchema>& members);
		CPPClassGenerator& addMethodDeclaration(MethodSchema method);
		CPPClassGenerator& addMethodDeclarations(const std::vector<MethodSchema>& methods);
		CPPClassGenerator& addMethodDefinition(MethodSchema method, String body);
		CPPClassGenerator& addMethodDefinition(MethodSchema method, const std::vector<String>& body);
		CPPClassGenerator& addTypeDefinition(String name, String type);
		CPPClassGenerator& finish();
		
		CPPClassGenerator& addDefaultConstructor();
		CPPClassGenerator& addConstructor(const std::vector<VariableSchema>& variables);
		CPPClassGenerator& addCustomConstructor(const std::vector<VariableSchema>& parameters, const std::vector<VariableSchema>& initialization);

		void writeTo(std::vector<String>& out, int nTabs = 0);
	private:
		String className;
		bool finished = false;
		CPPAccess currentAccess = CPPAccess::Private;

		std::vector<String> results;

		void ensureOK();
		static String getAccessString(CPPAccess access);
		static String getTypeString(TypeSchema type);
		static String getVariableString(VariableSchema var);
		static String getMethodSignatureString(MethodSchema var);
	};
}
