#include <lua/src/lua.hpp>
#include "halley/lua/lua_reference.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/lua/lua_state.h"
#include "halley/support/exception.h"
#include "halley/bytes/config_node_serializer.h"

using namespace Halley;

LuaReference::LuaReference()
	: lua(nullptr)
	, refId(LUA_NOREF)
{
}

LuaReference::LuaReference(LuaState& l, bool tracked)
	: lua(&l)
	, tracked(tracked)
{
	refId = luaL_ref(lua->getRawState(), LUA_REGISTRYINDEX);

	if (tracked) {
		lua->addTrackedReference(*this);
	}
}

LuaReference::LuaReference(LuaReference&& other) noexcept
{
	lua = other.lua;
	refId = other.refId;
	tracked = other.tracked;

	other.refId = LUA_NOREF;
	other.tracked = false;

	if (tracked) {
		lua->removeTrackedReference(other);
		lua->addTrackedReference(*this);
	}
}

LuaReference::~LuaReference()
{
	clear();
}

LuaReference& LuaReference::operator=(LuaReference&& other) noexcept
{
	if (this == &other) {
		return *this;
	}
	clear();

	lua = other.lua;
	refId = other.refId;
	tracked = other.tracked;

	other.refId = LUA_NOREF;
	other.tracked = false;

	if (tracked) {
		lua->removeTrackedReference(other);
		lua->addTrackedReference(*this);
	}

	return *this;
}

void LuaReference::pushToLuaStack() const
{
	Expects (refId != LUA_NOREF);
	Expects (lua);

	lua_rawgeti(lua->getRawState(), LUA_REGISTRYINDEX, refId);
}

void LuaReference::clear()
{
	if (refId != LUA_NOREF && lua != nullptr) {
		luaL_unref(lua->getRawState(), LUA_REGISTRYINDEX, refId);
		refId = LUA_NOREF;

		if (tracked) {
			lua->removeTrackedReference(*this);
			tracked = false;
		}
	}
}

void LuaReference::onStateDestroyed()
{
	tracked = false;
	refId = LUA_NOREF;
	lua = nullptr;
}

bool LuaReference::isValid() const
{
	return refId != LUA_NOREF;
}

bool LuaReference::isValid(const LuaState& state) const
{
	return isValid() && lua == &state;
}

LuaReference LuaReference::operator[](const String& name) const
{
	pushToLuaStack();
	lua_getfield(lua->getRawState(), -1, name.c_str());
	if (lua_isnil(lua->getRawState(), -1)) {
		throw Exception("Unknown field: " + name, HalleyExceptions::Lua);
	}
	lua_remove(lua->getRawState(), -2);
	return LuaReference(*lua);
}

LuaExpression::LuaExpression(String expr)
{
	setExpression(std::move(expr));
}

void LuaExpression::setExpression(String expr)
{
	expr.trimBoth();
	if (expression != expr) {
		expression = std::move(expr);
		luaRef.reset();
	}
}

bool LuaExpression::isEmpty() const
{
	return expression.isEmpty();
}

LuaReference& LuaExpression::get(LuaState& state) const
{
	if (!luaRef || !luaRef->isValid(state)) {
		auto stack = LuaStackOps(state);
		if (expression.startsWith("return") || expression.contains('\n')) {
			stack.load(expression);
		} else {
			stack.load("return " + expression);
		}
		luaRef = std::make_shared<LuaReference>(state, true);
	}
	return *luaRef;
}

void LuaExpression::serialize(Serializer& s) const
{
	s << expression;
}

void LuaExpression::deserialize(Deserializer& s)
{
	String expr;
	s >> expr;
	setExpression(expr);
}

ConfigNode ConfigNodeSerializer<LuaExpression>::serialize(const LuaExpression& expression, const EntitySerializationContext& context)
{
	return ConfigNode(expression.getExpression());
}

LuaExpression ConfigNodeSerializer<LuaExpression>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return LuaExpression(node.asString(""));
}

void ConfigNodeSerializer<LuaExpression>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, LuaExpression& target)
{
	target.setExpression(node.asString(""));
}
