#pragma once
#include "halley/maths/vector2.h"
#include "halley/data_structures/maybe.h"
#include <cstdint>
#include <cstddef>

namespace Halley {
	class String;
	class LuaState;

	class LuaStackOps {
	public:
		explicit LuaStackOps(LuaState& state);

		void push(std::nullptr_t n);
		void push(bool v);
		void push(int v);
		void push(int64_t v);
		void push(double v);
		void push(const String& v);
		void push(Vector2i v);

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

	private:
		LuaState& state;
	};
}
