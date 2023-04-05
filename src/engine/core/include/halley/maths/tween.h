#pragma once

#include "../utils/utils.h"
#include "../time/halleytime.h"

namespace Halley {
	enum class TweenCurve {
		Linear,
		Sinusoidal,
		Sqrt,
		Sine
	};

	template <>
	struct EnumNames<TweenCurve> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"linear",
				"sinusoidal",
				"sqrt",
				"sine"
			}};
		}
	};

	template <typename T>
	class Tween {
	public:

		static T applyCurve(T t, TweenCurve curve)
		{
			switch (curve) {
			case TweenCurve::Linear:
				return t;
			case TweenCurve::Sinusoidal:
				return smoothCos(t);
			case TweenCurve::Sqrt:
				return static_cast<T>(std::sqrt(t));
			case TweenCurve::Sine:
				return static_cast<T>(std::sin(t * static_cast<float>(pi()) * 0.5f));
			}
			return t;
		}

		Tween() {}

		Tween(T a, T b, Time length = 1.0, TweenCurve curve = TweenCurve::Linear)
			: a(std::move(a))
			, b(std::move(b))
			, length(length)
			, curve(curve)
		{}

		float getProgress() const
		{
			return float(time / length);
		}

		T getValue() const
		{
			return interpolate(a, b, applyCurve(getProgress(), curve));
		}

		void update(Time t)
		{
			time = clamp(time + t * direction, 0.0, length);
		}

		Tween& setLength(Time t)
		{
			length = t;
			return *this;
		}

		Tween& setStart(T _a)
		{
			a = std::move(_a);
			return *this;
		}

		Tween& setEnd(T _b)
		{
			b = std::move(_b);
			return *this;
		}

		Tween& resetTarget(T target)
		{
			a = getValue();
			b = std::move(target);
			time = 0;
			direction = 1;
			return *this;
		}

		Tween& setDirection(int dir)
		{
			direction = dir;
			return *this;
		}

		Tween& setCurve(TweenCurve c)
		{
			curve = c;
			return *this;
		}

	private:
		T a;
		T b;
		Time time = 0.0;
		Time length = 1.0;
		int direction = 0;
		TweenCurve curve = TweenCurve::Linear;
	};
}