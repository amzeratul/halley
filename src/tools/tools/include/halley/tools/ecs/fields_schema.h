#pragma once

#include <halley/text/halleystring.h>
#include <halley/text/string_converter.h>
#include <array>
#include <optional>
#include <utility>

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
			: name(std::move(name))
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
			: type(std::move(type))
			, name(std::move(name))
			, initialValue(std::move(initialValue))
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
		std::vector<String> defaultValue;
		MemberAccess access;
		bool serializable;
		bool collapse;

		MemberSchema(TypeSchema type, String name, std::vector<String> defaultValue, MemberAccess access = MemberAccess::Public, bool serializable = true, bool collapse = false)
			: type(std::move(type))
			, name(std::move(name))
			, defaultValue(std::move(defaultValue))
			, access(access)
			, serializable(serializable)
			, collapse(collapse)
		{}

		MemberSchema(TypeSchema type, String name, String defaultValue = "", MemberAccess access = MemberAccess::Public, bool serializable = true, bool collapse = false)
			: MemberSchema(std::move(type), std::move(name), defaultValue.isEmpty() ? std::vector<String>() : std::vector<String>{std::move(defaultValue)}, access, serializable, collapse)
		{}

		static std::vector<VariableSchema> toVariableSchema(const std::vector<MemberSchema>& schema)
		{
			std::vector<VariableSchema> result;
			result.reserve(schema.size());
			for (const auto& s: schema) {
				result.emplace_back(s.type, s.name);
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
		bool isPure = false;

		MethodSchema(TypeSchema returnType, Vector<VariableSchema> arguments, String name, bool isConst = false, bool isVirtual = false, bool isOverride = false, bool isFinal = false, bool isFriend = false, bool isPure = false)
			: returnType(std::move(returnType))
			, arguments(std::move(arguments))
			, name(std::move(name))
			, isConst(isConst)
			, isVirtual(isVirtual)
			, isOverride(isOverride)
			, isFinal(isFinal)
			, isFriend(isFriend)
			, isPure(isPure)
		{}
	};
}
