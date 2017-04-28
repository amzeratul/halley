#include <lua.hpp>
#include "lua_reference.h"
#include "lua_state.h"
#include "halley/support/exception.h"

using namespace Halley;

LuaReference::LuaReference()
	: lua(nullptr)
	, refId(LUA_NOREF)
{
}

LuaReference::LuaReference(LuaState& l)
	: lua(&l)
{
	refId = luaL_ref(lua->getRawState(), LUA_REGISTRYINDEX);
}

LuaReference::LuaReference(LuaReference&& other) noexcept
{
	lua = other.lua;
	refId = other.refId;
	other.refId = LUA_NOREF;
}

LuaReference& LuaReference::operator=(LuaReference&& other) noexcept
{
	lua = other.lua;
	refId = other.refId;
	other.refId = LUA_NOREF;
	return *this;
}

LuaReference::~LuaReference()
{
	if (refId != LUA_NOREF && lua!= nullptr) {
		luaL_unref(lua->getRawState(), LUA_REGISTRYINDEX, refId);
	}
}

void LuaReference::pushToLuaStack() const
{
	Expects (refId != LUA_NOREF);
	Expects (lua);

	lua_rawgeti(lua->getRawState(), LUA_REGISTRYINDEX, refId);
}

LuaReference LuaReference::operator[](const String& name) const
{
	pushToLuaStack();
	lua_getfield(lua->getRawState(), -1, name.c_str());
	if (lua_isnil(lua->getRawState(), -1)) {
		throw Exception("Unknown field: " + name);
	}
	lua_remove(lua->getRawState(), -2);
	return LuaReference(*lua);
}
