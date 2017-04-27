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

		void push(std::nullptr_t);
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
		void getField(const String& name);

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


	// Generic from/to
	template <typename T>
	struct FromLua {
		inline T operator()(LuaState& state) const = delete;
	};

	template <typename T>
	struct ToLua {
		inline void operator()(LuaState& state, const T& value) const { LuaStackOps(state).push(value); }
	};


	// Specialisations
	template <>
	struct FromLua<void> {
		inline void operator()(LuaState&) const {};
	};

	template <>
	struct FromLua<bool> {
		inline bool operator()(LuaState& state) const { return LuaStackOps(state).popBool(); };
	};

	template <>
	struct FromLua<int> {
		inline int operator()(LuaState& state) const { return LuaStackOps(state).popInt(); };
	};

	template <>
	struct FromLua<int64_t> {
		inline int64_t operator()(LuaState& state) const { return LuaStackOps(state).popInt64(); };
	};

	template <>
	struct FromLua<double> {
		inline double operator()(LuaState& state) const { return LuaStackOps(state).popDouble(); };
	};

	template <>
	struct FromLua<String> {
		inline String operator()(LuaState& state) const { return LuaStackOps(state).popString(); };
	};

	template <>
	struct FromLua<Vector2i> {
		inline Vector2i operator()(LuaState& state) const { return LuaStackOps(state).popVector2i(); };
	};

	template <>
	struct FromLua<LuaTable> {
		inline LuaTable operator()(LuaState& state) const { return LuaStackOps(state).popTable(); };
	};

	template <>
	struct FromLua<LuaState&> {
		inline LuaState& operator()(LuaState& state) const { return state; };
	};


	template <typename T>
	struct FromLua<Maybe<T>> {
		inline Maybe<T> operator()(LuaState& state) const {
			if (LuaStackOps(state).isTopNil()) {
				return Maybe<T>();
			} else {
				return FromLua<T>()(state);
			}
		}
	};

	template <typename T>
	struct ToLua<Maybe<T>> {
		inline void operator()(LuaState& state, const Maybe<T>& value) const {
			if (value) {
				LuaStackOps(state).push(value.get());
			} else {
				LuaStackOps(state).push(nullptr);
			}
		}
	};


	// Utils
	class LuaStackUtils {
	public:
		LuaStackUtils(LuaState& state) : state(state) {}

		template <typename T>
		inline void push(T value)
		{
			ToLua<T>()(state, value);
		}

		template <typename T>
		inline void pop()
		{
			return FromLua<T>()(state);
		}
		
		template <typename T>
		void setField(const String& name, T v)
		{
			push<T>(v);
			LuaStackOps(state).setField(name);
		}

		template <typename T>
		T getField(const String& name)
		{
			LuaStackOps(state).getField(name);
			return pop<T>();
		}

		void makeGlobal(const String& name)
		{
			LuaStackOps(state).makeGlobal(name);
		}

	private:
		LuaState& state;
	};
}
