#pragma once
#include "halley/maths/vector2.h"
#include "halley/data_structures/maybe.h"
#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace Halley {
	class String;
	class LuaState;

	using LuaCallback = std::function<int(LuaState&)>;

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
		void pushTable(int nArrayIndices = 0, int nRecords = 0);

		void makeGlobal(const String& name);

		void setField(const String& name);
		void setField(int idx);
		void getField(const String& name);
		void getField(int idx);

		void pop();
		bool popBool();
		int popInt();
		int64_t popInt64();
		double popDouble();
		String popString();
		Vector2i popVector2i();
		
		bool isTopNil();

	private:
		LuaState& state;
	};

	class LuaCustomSerialize {};

	namespace StdLuaConversion {
		template <typename T, std::enable_if_t<std::is_base_of<LuaCustomSerialize, T>::value, int> = 0>
		inline T from(LuaState& state)
		{
			T result;
			result.fromLua(state);
			return result;
		}
    
		template <typename T, std::enable_if_t<!std::is_base_of<LuaCustomSerialize, T>::value, int> = 0>
		inline T from(LuaState& state)
		{
			return T();
		}
		
		template <typename T, std::enable_if_t<std::is_base_of<LuaCustomSerialize, T>::value, int> = 0>
		inline void to(LuaState& state, const T& value)
		{
			value.toLua(state);
		}
    
		template <typename T, std::enable_if_t<!std::is_base_of<LuaCustomSerialize, T>::value, int> = 0>
		inline void to(LuaState& state, const T& value)
		{
			LuaStackOps(state).push(value);
		}
	}

	
	// Generic from/to
	template <typename T>
	struct FromLua {
		inline T operator()(LuaState& state) const { return StdLuaConversion::from<T>(state); }
	};


	template <typename T>
	struct ToLua {
		inline void operator()(LuaState& state, const T& value) const { StdLuaConversion::to<T>(state, value); }
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
				ToLua<T>()(state, value.get());
			} else {
				LuaStackOps(state).push(nullptr);
			}
		}
	};

	template <typename T>
	struct FromLua<std::vector<T>> {
		inline std::vector<T> operator()(LuaState& state) const {
			return {}; // TODO
		}
	};

	template <typename T>
	struct ToLua<std::vector<T>> {
		inline void operator()(LuaState& state, const std::vector<T>& value) const {
			auto ops = LuaStackOps(state);
			ops.pushTable(0, int(value.size()));
			for (size_t i = 0; i < value.size(); ++i) {
				ToLua<T>()(state, value[i]);
				ops.setField(int(i + 1));
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
		inline T pop()
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
		void setField(int idx, T v)
		{
			push<T>(v);
			LuaStackOps(state).setField(idx);
		}

		template <typename T>
		T getField(const String& name)
		{
			LuaStackOps(state).getField(name);
			return pop<T>();
		}

		template <typename T>
		T getField(int idx)
		{
			LuaStackOps(state).getField(idx);
			return pop<T>();
		}

		template <typename T>
		void getField(const String& name, T& v)
		{
			v = getField<T>(name);
		}

		void makeGlobal(const String& name)
		{
			LuaStackOps(state).makeGlobal(name);
		}

		void pushTable(int nArr = 0, int nRec = 0)
		{
			LuaStackOps(state).pushTable(nArr, nRec);
		}

	private:
		LuaState& state;
	};
}
