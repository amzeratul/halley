#include "../../../../../contrib/lua/src/lua.hpp"
#include "lua_stack_ops.h"
#include "lua_state.h"

using namespace Halley;

void LuaStackOps::push(std::nullptr_t)
{
	lua_pushnil(state.getRawState());
}

void LuaStackOps::push(bool v)
{
	lua_pushboolean(state.getRawState(), v);
}

void LuaStackOps::push(int v)
{
	lua_pushinteger(state.getRawState(), v);
}

void LuaStackOps::push(int64_t v)
{
	lua_pushinteger(state.getRawState(), v);
}

void LuaStackOps::push(double v)
{
	lua_pushnumber(state.getRawState(), v);
}

void LuaStackOps::push(const char* v)
{
	lua_pushstring(state.getRawState(), v);
}

void LuaStackOps::push(const String& v)
{
	lua_pushstring(state.getRawState(), v.c_str());
}

void LuaStackOps::push(Vector2i v)
{
	lua_createtable(state.getRawState(), 0, 2);
	push(v.x);
	lua_setfield(state.getRawState(), -2, "x");
	push(v.y);
	lua_setfield(state.getRawState(), -2, "y");
}

void LuaStackOps::push(LuaCallback callback)
{
	state.pushCallback(std::move(callback));
}

void LuaStackOps::push(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Undefined) {
		push(nullptr);
	} else if (node.getType() == ConfigNodeType::Float) {
		push(static_cast<double>(node.asFloat()));
	} else if (node.getType() == ConfigNodeType::Int) {
		push(node.asInt());
	} else if (node.getType() == ConfigNodeType::Bool) {
		push(node.asBool());
	} else if (node.getType() == ConfigNodeType::String) {
		push(node.asString());
	} else if (node.getType() == ConfigNodeType::Sequence) {
		push(node.asSequence());
	} else if (node.getType() == ConfigNodeType::Map) {
		push(node.asMap());
	} else {
		throw Exception("Unimplemented ConfigNode to Lua serialization: " + toString(node.getType()), HalleyExceptions::Lua);
	}
}

void LuaStackOps::pushTable(int nArrayIndices, int nRecords)
{
	lua_createtable(state.getRawState(), nArrayIndices, nRecords);
}

void LuaStackOps::load(const String& v, const String& name)
{
	luaL_loadbufferx(state.getRawState(), v.c_str(), v.length(), name.isEmpty() ? nullptr : name.c_str(), "t");
}

void LuaStackOps::load(gsl::span<const gsl::byte> bytes, const String& name)
{
	luaL_loadbufferx(state.getRawState(), reinterpret_cast<const char*>(bytes.data()), bytes.size_bytes(), name.isEmpty() ? nullptr : name.c_str(), "b");
}

void LuaStackOps::load(const Bytes& bytes, const String& name)
{
	load(gsl::as_bytes(gsl::span<const Byte>(bytes)), name);
}

void LuaStackOps::eval(const String& v, const String& name)
{
	load(v, name);
	state.call(0, 1);
}

void LuaStackOps::eval(gsl::span<const gsl::byte> bytes, const String& name)
{
	load(bytes, name);
	state.call(0, 1);
}

void LuaStackOps::eval(const Bytes& bytes, const String& name)
{
	load(bytes, name);
	state.call(0, 1);
}

namespace {
	int luaWriter(lua_State* L, const void* p, size_t sz, void* dstVoid)
	{
		auto dst = static_cast<Bytes*>(dstVoid);
		const size_t startSize = dst->size();
		dst->resize(startSize + sz);
		memcpy(reinterpret_cast<char*>(dst->data()) + startSize, p, sz);
		return 0;
	}
}

Bytes LuaStackOps::compile(const String& v, bool stripDebug)
{
	Bytes data;
	luaL_loadbuffer(state.getRawState(), v.c_str(), v.length(), nullptr);
	lua_dump(state.getRawState(), &luaWriter, &data, stripDebug);
	pop();
	return data;
}

Bytes LuaStackOps::compileAndEval(const String& v, const String& name, bool stripDebug)
{
	Bytes data;
	luaL_loadbuffer(state.getRawState(), v.c_str(), v.length(), name.isEmpty() ? nullptr : name.c_str());
	lua_dump(state.getRawState(), &luaWriter, &data, stripDebug);
	state.call(0, 1);
	return data;
}

void LuaStackOps::makeGlobal(const String& name)
{
	lua_setglobal(state.getRawState(), name.c_str());
}

void LuaStackOps::pop()
{
	lua_pop(state.getRawState(), 1);
}

bool LuaStackOps::popBool()
{
	auto value = lua_toboolean(state.getRawState(), -1) != 0;
	pop();
	return value;
}

int LuaStackOps::popInt()
{
	auto value = lua_tointeger(state.getRawState(), -1);
	pop();
	return int(value);
}

int64_t LuaStackOps::popInt64()
{
	auto value = lua_tointeger(state.getRawState(), -1);
	pop();
	return value;
}

double LuaStackOps::popDouble()
{
	auto value = lua_tonumber(state.getRawState(), -1);
	pop();
	return value;
}

String LuaStackOps::popString()
{
	String value = lua_tostring(state.getRawState(), -1);
	pop();
	return value;
}

Vector2i LuaStackOps::popVector2i()
{
	if (!lua_istable(state.getRawState(), -1)) {
		throw Exception("Invalid value at Lua stack", HalleyExceptions::Lua);
	}
	Vector2i result;
	lua_getfield(state.getRawState(), -1, "x");
	result.x = popInt();
	lua_getfield(state.getRawState(), -1, "y");
	result.y = popInt();
	pop();
	return result;
}

ConfigNode LuaStackOps::popConfigNode()
{
	auto type = lua_type(state.getRawState(), -1);
	switch (type) {
	case LUA_TNIL:
		pop();
		return ConfigNode();
	case LUA_TNUMBER:
		return ConfigNode(float(popDouble()));
	case LUA_TBOOLEAN:
		return ConfigNode(popBool());
	case LUA_TSTRING:
	case LUA_TTABLE:
	case LUA_TFUNCTION:
	case LUA_TUSERDATA:
	case LUA_TTHREAD:
	case LUA_TLIGHTUSERDATA:
		return ConfigNode(popString());
	}
	return ConfigNode();
}

bool LuaStackOps::isTopNil()
{
	return lua_isnil(state.getRawState(), -1);
}

int LuaStackOps::getLength()
{
	return int(lua_rawlen(state.getRawState(), -1));
}

void LuaStackOps::setField(const String& name)
{
	lua_setfield(state.getRawState(), -2, name.c_str());
}

void LuaStackOps::setField(int idx)
{
	lua_rawseti(state.getRawState(), -2, idx);
}

void LuaStackOps::getField(const String& name)
{
	lua_getfield(state.getRawState(), -1, name.c_str());
}

void LuaStackOps::getField(int idx)
{
	lua_rawgeti(state.getRawState(), -1, idx);
}
