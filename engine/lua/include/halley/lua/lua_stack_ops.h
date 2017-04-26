#pragma once
#include "halley/maths/vector2.h"
#include "halley/data_structures/maybe.h"
#include <cstdint>
#include <cstddef>

namespace Halley {
	class String;
	class LuaState;

	using LuaCallback = std::function<int(LuaState&)>;

	class LuaTable {}; // TODO

	class LuaStackOps {
	public:
		inline explicit LuaStackOps(LuaState& state) : state(state) {}

		void push(std::nullptr_t n);
		void push(bool v);
		void push(int v);
		void push(int64_t v);
		void push(double v);
		void push(const char* v);
		void push(const String& v);
		void push(Vector2i v);
		void push(LuaCallback callback);
		void push(const LuaTable& table);

		void makeGlobal(const String& name);

		void setField(const String& name);

		template <typename T>
		void setField(const String& name, T v)
		{
			push(v);
			setField(name);
		}

		template <typename T>
		void push(Maybe<T> v)
		{
			if (v) {
				push(v.get());
			} else {
				push(nullptr);
			}
		}

		void pop();
		bool popBool();
		int popInt();
		int64_t popInt64();
		double popDouble();
		String popString();
		Vector2i popVector2i();
		LuaTable popTable();
		
		bool isTopNil();

	private:
		LuaState& state;
	};

	class LuaStackReturn {
	public:
		inline explicit LuaStackReturn(LuaState& state) : state(state) {}

		operator bool() const { return LuaStackOps(state).popBool(); }
		operator int() const { return LuaStackOps(state).popInt(); }
		operator int64_t() const { return LuaStackOps(state).popInt64(); }
		operator double() const { return LuaStackOps(state).popDouble(); }
		operator String() const { return LuaStackOps(state).popString(); }
		operator Vector2i() const { return LuaStackOps(state).popVector2i(); }
		operator LuaTable() const { return LuaStackOps(state).popTable(); }
		operator LuaState&() const { return state; }

		template <typename T>
		operator Maybe<T>() const
		{
			LuaStackOps stack(state);
			if (stack.isTopNil()) {
				return Maybe<T>();
			} else {
				return T(*this);
			}
		}

	private:
		LuaState& state;
	};
}
