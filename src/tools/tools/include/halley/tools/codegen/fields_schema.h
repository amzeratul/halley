#pragma once

#include <halley/text/halleystring.h>
#include <halley/text/string_converter.h>
#include <array>

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

	enum class MemberAccess {
		Public,
		Protected,
		Private
	};

	template <>
	struct EnumNames<MemberAccess> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"public",
				"protected",
				"private"
			}};
		}
	};

	class MemberSchema
	{
	public:
		TypeSchema type;
		String name;
		String defaultValue;
		MemberAccess access;

		MemberSchema(TypeSchema type, String name, String defaultValue = "", MemberAccess access = MemberAccess::Public)
			: type(type)
			, name(name)
			, defaultValue(defaultValue)
			, access(access)
		{}

		static std::vector<VariableSchema> toVariableSchema(const std::vector<MemberSchema>& schema)
		{
			std::vector<VariableSchema> result;
			result.reserve(schema.size());
			for (auto& s: schema) {
				result.push_back(VariableSchema(s.type, s.name));
			}
			return result;
		}
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
