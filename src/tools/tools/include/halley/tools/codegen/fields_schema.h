#pragma once

#include <halley/text/halleystring.h>

namespace Halley
{
	class TypeSchema
	{
	public:
		String name;
		bool isConst = false;
		bool isStatic = false;
		bool isConstExpr = false;

		TypeSchema(String name, bool isConst = false, bool isStatic = false, bool isConstExpr = false)
			: name(name)
			, isConst(isConst)
			, isStatic(isStatic)
			, isConstExpr(isConstExpr)
		{}
	};

	class VariableSchema
	{
	public:
		TypeSchema type;
		String name;
		String initialValue;

		VariableSchema(TypeSchema type, String name, String initialValue = "")
			: type(type)
			, name(name)
			, initialValue(initialValue)
		{}
	};

	class MethodSchema
	{
	public:
		TypeSchema returnType;
		Vector<VariableSchema> arguments;
		String name;
		bool isConst = false;
		bool isVirtual = false;
		bool isOverride = false;
		bool isFinal = false;
		bool isFriend = false;

		MethodSchema(TypeSchema returnType, Vector<VariableSchema> arguments, String name, bool isConst = false, bool isVirtual = false, bool isOverride = false, bool isFinal = false, bool isFriend = false)
			: returnType(returnType)
			, arguments(arguments)
			, name(name)
			, isConst(isConst)
			, isVirtual(isVirtual)
			, isOverride(isOverride)
			, isFinal(isFinal)
			, isFriend(isFriend)
		{}
	};
}