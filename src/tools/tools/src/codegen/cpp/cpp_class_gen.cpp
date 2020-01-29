#include <halley/support/exception.h>
#include "cpp_class_gen.h"

using namespace Halley;

CPPClassGenerator::CPPClassGenerator(String name)
	: className(name) 
{
	results.push_back("class " + name + " {");
}

CPPClassGenerator::CPPClassGenerator(String name, String baseClass, MemberAccess inheritanceType, bool isFinal)
	: className(name)
{
	results.push_back("class " + name + (isFinal ? " final" : "") + " : " + toString(inheritanceType) + " " + baseClass + " {");
}

CPPClassGenerator& CPPClassGenerator::addClass(CPPClassGenerator& otherClass)
{
	ensureOK();
	otherClass.writeTo(results, 1);
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addBlankLine()
{
	return addLine("");
}

CPPClassGenerator& CPPClassGenerator::addLine(String line)
{
	ensureOK();
	results.push_back("\t" + line);
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addComment(String comment)
{
	ensureOK();
	results.push_back("\t// " + comment);
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addAccessLevelSection(MemberAccess access)
{
	ensureOK();
	if (currentAccess != access) {
		currentAccess = access;
		results.push_back(toString(access) + ":");
	}
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMember(MemberSchema member)
{
	addAccessLevelSection(member.access);
	results.push_back("\t" + getMemberString(member) + ";");
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMembers(const Vector<MemberSchema>& members)
{
	for (auto& m : members) {
		addMember(m);
	}
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMethodDeclaration(MethodSchema method)
{
	ensureOK();
	results.push_back("\t" + getMethodSignatureString(method) + ";");
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMethodDeclarations(const Vector<MethodSchema>& methods)
{
	ensureOK();
	for (auto& m : methods) {
		addMethodDeclaration(m);
	}
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMethodDefinition(MethodSchema method, String body)
{
	return addMethodDefinition(method, Vector<String>{ body });
}

CPPClassGenerator& CPPClassGenerator::addMethodDefinition(MethodSchema method, const Vector<String>& body)
{
	ensureOK();
	results.push_back("\t" + getMethodSignatureString(method) + " {");
	for (auto& line : body) {
		results.push_back("\t\t" + line);
	}
	results.push_back("\t}");
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addTypeDefinition(String name, String type)
{
	ensureOK();
	results.push_back("\tusing " + name + " = " + type + ";");
	return *this;
}

CPPClassGenerator& CPPClassGenerator::finish()
{
	ensureOK();
	results.push_back("};");
	finished = true;
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addDefaultConstructor()
{
	return addCustomConstructor({}, {});
}

CPPClassGenerator& CPPClassGenerator::addConstructor(const Vector<VariableSchema>& variables)
{
	Vector<VariableSchema> init;
	for (auto& v : variables) {
		init.push_back(v);
		init.back().initialValue = v.name;
	}
	return addCustomConstructor(variables, init);
}

CPPClassGenerator& CPPClassGenerator::addCustomConstructor(const Vector<VariableSchema>& parameters, const Vector<VariableSchema>& initialization)
{
	String sig = "\t" + getMethodSignatureString(MethodSchema(TypeSchema(""), parameters, className));
	String body = "{}";

	if (initialization.size() > 0) {
		results.push_back(sig);
		bool first = true;
		for (auto& i: initialization) {
			String prefix = first ? "\t\t: ": "\t\t, ";
			first = false;
			results.push_back(prefix + i.name + "(" + i.initialValue + ")");
		}
		results.push_back("\t" + body);
	} else {
		results.push_back(sig + " " + body);
	}
	return *this;
}

void CPPClassGenerator::writeTo(Vector<String>& out, int nTabs)
{
	if (!finished) {
		throw Exception("Class not finished yet.", HalleyExceptions::Tools);
	}

	String prefix;
	for (int i = 0; i < nTabs; i++) {
		prefix += "\t";
	}

	for (size_t i = 0; i < results.size(); i++) {
		out.push_back(prefix + results[i]);
	}
}

void CPPClassGenerator::ensureOK() const
{
	if (finished) {
		throw Exception("finish() has already been called!", HalleyExceptions::Tools);
	}
}

String CPPClassGenerator::getTypeString(TypeSchema type)
{
	String value;
	if (type.isStatic) {
		value += "static ";
	}
	if (type.isConst) {
		value += "const ";
	}
	if (type.isConstExpr) {
		value += "constexpr ";
	}
	value += type.name;
	return value;
}

String CPPClassGenerator::getVariableString(VariableSchema var)
{
	String init = "";
	if (var.initialValue != "") {
		init = " = " + var.initialValue;
	}
	return getTypeString(var.type) + " " + var.name + init;
}

String CPPClassGenerator::getMemberString(MemberSchema var)
{
	String init = "";
	if (var.defaultValue != "") {
		init = " = " + var.defaultValue;
	}
	return getTypeString(var.type) + " " + var.name + init;
}

String CPPClassGenerator::getMethodSignatureString(MethodSchema method)
{
	Vector<String> args;
	for (auto& a : method.arguments) {
		args.push_back(getVariableString(a));
	}
	String returnType = getTypeString(method.returnType);
	if (returnType != "") {
		returnType += " ";
	}

	return (method.isFriend ? "friend " : "") + returnType + method.name + "(" + String::concatList(args, ", ") + ")" + (method.isConst ? " const" : "") + (method.isOverride ? " override" : "") + (method.isFinal ? " final" : "");

}
