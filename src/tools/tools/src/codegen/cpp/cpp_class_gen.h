#pragma once

#include <halley/tools/codegen/fields_schema.h>

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
		CPPClassGenerator& addMembers(const Vector<VariableSchema>& members);
		CPPClassGenerator& addMethodDeclaration(MethodSchema method);
		CPPClassGenerator& addMethodDeclarations(const Vector<MethodSchema>& methods);
		CPPClassGenerator& addMethodDefinition(MethodSchema method, String body);
		CPPClassGenerator& addMethodDefinition(MethodSchema method, const Vector<String>& body);
		CPPClassGenerator& addTypeDefinition(String name, String type);
		CPPClassGenerator& finish();
		
		CPPClassGenerator& addDefaultConstructor();
		CPPClassGenerator& addConstructor(const Vector<VariableSchema>& variables);
		CPPClassGenerator& addCustomConstructor(const Vector<VariableSchema>& parameters, const Vector<VariableSchema>& initialization);
		
		static String getAccessString(CPPAccess access);
		static String getTypeString(TypeSchema type);
		static String getVariableString(VariableSchema var);
		static String getMethodSignatureString(MethodSchema var);

		void writeTo(Vector<String>& out, int nTabs = 0);
	private:
		String className;
		bool finished = false;
		CPPAccess currentAccess = CPPAccess::Private;

		Vector<String> results;

		void ensureOK() const;
	};
}
