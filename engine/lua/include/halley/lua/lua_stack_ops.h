#pragma once
#include "halley/maths/vector2.h"
#include "halley/data_structures/maybe.h"
#include <cstdint>
#include <cstddef>

namespace Halley {
	class String;
	class LuaState;

	using LuaCallback = std::function<int(LuaState&)>;

	class LuaStackOps {
	public:
		explicit LuaStackOps(LuaState& state);

		void push(std::nullptr_t n);
		void push(bool v);
		void push(int v);
		void push(int64_t v);
		void push(double v);
		void push(const char* v);
		void push(const String& v);
		void push(Vector2i v);
		void push(LuaCallback callback);

		void pushTable();
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

	private:
		LuaState& state;
	};

	class LuaStackReturn {
	public:
		explicit LuaStackReturn(LuaState& state);

		operator void() const;
		operator bool() const;
		operator int() const;
		operator int64_t() const;
		operator double() const;
		operator String() const;
		operator Vector2i() const;
		operator LuaState&() const;

	private:
		LuaState& state;
	};
}
