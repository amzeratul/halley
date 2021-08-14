#pragma once

#include <halley/text/halleystring.h>
#include <halley/text/string_converter.h>
#include <array>
#include <optional>
#include <utility>

#include "halley/maths/range.h"

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
		std::optional<MemberAccess> access;

		MemberSchema(TypeSchema type, String name, std::vector<String> defaultValue, std::optional<MemberAccess> access = {})
			: type(std::move(type))
			, name(std::move(name))
			, defaultValue(std::move(defaultValue))
			, access(access)
		{}

		MemberSchema(TypeSchema type, String name, String defaultValue = "", std::optional<MemberAccess> access = {})
			: MemberSchema(std::move(type), std::move(name), defaultValue.isEmpty() ? std::vector<String>() : std::vector<String>{std::move(defaultValue)}, access)
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

		String getValueString(bool initializer = true) const;
	};

	class ComponentFieldSchema : public MemberSchema {
	public:
		String displayName;
		bool canSave = true;
		bool canEdit = true;
		bool collapse = false;
		std::optional<Range<float>> range;

		ComponentFieldSchema(TypeSchema type, String name, std::vector<String> defaultValue, std::optional<MemberAccess> access = {})
			: MemberSchema(std::move(type), std::move(name), std::move(defaultValue), access)
		{}

		ComponentFieldSchema(TypeSchema type, String name, String defaultValue = "", std::optional<MemberAccess> access = {})
			: MemberSchema(std::move(type), std::move(name), std::move(defaultValue), access)
		{}

		static std::vector<VariableSchema> toVariableSchema(const std::vector<ComponentFieldSchema>& schema)
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
