#pragma once

#include <halley/tools/ecs/fields_schema.h>

namespace Halley
{
	class CPPClassGenerator
	{
	public:
		explicit CPPClassGenerator(String name);
		explicit CPPClassGenerator(String name, String baseClass, MemberAccess inheritanceType = MemberAccess::Public, bool isFinal = false);

		CPPClassGenerator& addBlankLine();
		CPPClassGenerator& addLine(String line);

		CPPClassGenerator& addClass(CPPClassGenerator& otherClass);
		CPPClassGenerator& addComment(String comment);
		CPPClassGenerator& addAccessLevelSection(MemberAccess access);
		CPPClassGenerator& addMember(MemberSchema member);
		CPPClassGenerator& addMembers(const Vector<MemberSchema>& members);
		CPPClassGenerator& addMethodDeclaration(MethodSchema method);
		CPPClassGenerator& addMethodDeclarations(const Vector<MethodSchema>& methods);
		CPPClassGenerator& addMethodDefinition(MethodSchema method, String body);
		CPPClassGenerator& addMethodDefinition(MethodSchema method, const Vector<String>& body);
		CPPClassGenerator& addTypeDefinition(String name, String type);
		CPPClassGenerator& finish();
		
		CPPClassGenerator& addDefaultConstructor();
		CPPClassGenerator& addConstructor(const Vector<VariableSchema>& variables, bool move);
		CPPClassGenerator& addCustomConstructor(const Vector<VariableSchema>& parameters, const Vector<VariableSchema>& initialization);
		
		static String getTypeString(const TypeSchema& type);
		static String getVariableString(const VariableSchema& var);
		static String getMemberString(const MemberSchema& var);
		static String getMethodSignatureString(const MethodSchema& var);

		void writeTo(Vector<String>& out, int nTabs = 0);
	private:
		String className;
		bool finished = false;
		MemberAccess currentAccess = MemberAccess::Private;
		std::optional<MemberAccess> pendingAccess;

		Vector<String> results;

		void ensureOK() const;
		void flushAccess();
	};
}
