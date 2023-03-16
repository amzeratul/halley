#include <lua/src/lua.hpp>
#include "halley/lua/lua_reference.h"
#include "halley/lua/lua_state.h"
#include "halley/support/exception.h"

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
