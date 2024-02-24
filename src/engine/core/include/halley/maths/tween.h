#pragma once

#include "../utils/utils.h"
#include "../time/halleytime.h"

namespace Halley {
	enum class TweenCurve {
		Linear,
		Sine,
		SineIn,
		SineOut,
		Quadratic,
		QuadraticIn,
		QuadraticOut,
		Cubic,
		CubicIn,
		CubicOut,
		Sqrt,
		SqrtIn,
		SqrtOut,
		EaseBack,
		EaseBackIn,
		EaseBackOut
	};

	template <>
	struct EnumNames<TweenCurve> {
		constexpr std::array<const char*, 16> operator()() const {
			return{{
				"linear",
				"sine",
				"sineIn",
				"sineOut",
				"quadratic",
				"quadraticIn",
				"quadraticOut",
				"cubic",
				"cubicIn",
				"cubicOut",
				"sqrt",
				"sqrtIn",
				"sqrtOut",
				"easeBack",
				"easeBackIn",
				"easeBackOut",
			}};
		}
	};

	template <typename T>
	class Tween {
	public:

		template <TweenCurve curve>
		constexpr static T applyInverseCurve(T t)
		{
			return 1.0f - Tween<T>::applyCurve<curve>(1.0f - t);
		}

		template <TweenCurve curve>
		constexpr static T applyCurve(T t)
		{
			if constexpr (curve == TweenCurve::Linear) {
				return t;
			} else if constexpr (curve == TweenCurve::Sine) {
				return (1.0f - std::cos(t * static_cast<float>(pi()))) * 0.5f;
			} else if constexpr (curve == TweenCurve::SineIn) {
				return applyInverseCurve<TweenCurve::SineOut>(t);
			} else if constexpr (curve == TweenCurve::SineOut) {
				return static_cast<T>(std::sin(t * static_cast<float>(pi()) * 0.5f));
			} else if constexpr (curve == TweenCurve::Quadratic) {
				return t < 0.5f ?
					2.0f * applyCurve<TweenCurve::QuadraticIn>(t) :
					1.0f - applyCurve<TweenCurve::QuadraticIn>(-2.0f * t + 2.0f) * 0.5f;
			} else if constexpr (curve == TweenCurve::QuadraticIn) {
				return t * t;
			} else if constexpr (curve == TweenCurve::QuadraticOut) {
				return applyInverseCurve<TweenCurve::QuadraticIn>(t);
			} else if constexpr (curve == TweenCurve::Cubic) {
				return t < 0.5f ?
					4.0f * applyCurve<TweenCurve::CubicIn>(t) :
					1.0f - applyCurve<TweenCurve::CubicIn>(-2.0f * t + 2.0f) * 0.5f;
			} else if constexpr (curve == TweenCurve::CubicIn) {
				return t * t * t;
			} else if constexpr (curve == TweenCurve::CubicOut) {
				return applyInverseCurve<TweenCurve::CubicIn>(t);
			} else if constexpr (curve == TweenCurve::Sqrt) {
				return t < 0.5f ?
					applyCurve<TweenCurve::SqrtIn>(t) / sqrt(2.0f) :
					1.0f - applyCurve<TweenCurve::SqrtIn>(-2.0f * t + 2.0f) * 0.5f;
			} else if constexpr (curve == TweenCurve::SqrtIn) {
				return static_cast<T>(std::sqrt(t));
			} else if constexpr (curve == TweenCurve::SqrtOut) {
				return applyInverseCurve<TweenCurve::SqrtIn>(t);
			} else if constexpr (curve == TweenCurve::EaseBack) {
				return t < 0.5f ?
					applyCurve<TweenCurve::EaseBackIn>(t) / sqrt(2.0f) :
					1.0f - applyCurve<TweenCurve::EaseBackOut>(-2.0f * t + 2.0f) * 0.5f;
			} else if constexpr (curve == TweenCurve::EaseBackIn) {
				const static double c1 = 1.70158;
				const static double c3 = c1 + 1.0;
				return static_cast<T>(c3 * t * t * t - c1 * t * t);
			} else if constexpr (curve == TweenCurve::EaseBackOut) {
				return applyInverseCurve<TweenCurve::EaseBackIn>(t);
			}
			return t;
		}

		constexpr static T applyCurve(T t, TweenCurve curve)
		{
			switch (curve) {
			case TweenCurve::Linear:
				return Tween<T>::applyCurve<TweenCurve::Linear>(t);
			case TweenCurve::Sine:
				return Tween<T>::applyCurve<TweenCurve::Sine>(t);
			case TweenCurve::SineIn:
				return Tween<T>::applyCurve<TweenCurve::SineIn>(t);
			case TweenCurve::SineOut:
				return Tween<T>::applyCurve<TweenCurve::SineOut>(t);
			case TweenCurve::Quadratic:
				return Tween<T>::applyCurve<TweenCurve::Quadratic>(t);
			case TweenCurve::QuadraticIn:
				return Tween<T>::applyCurve<TweenCurve::QuadraticIn>(t);
			case TweenCurve::QuadraticOut:
				return Tween<T>::applyCurve<TweenCurve::QuadraticOut>(t);
			case TweenCurve::Cubic:
				return Tween<T>::applyCurve<TweenCurve::Cubic>(t);
			case TweenCurve::CubicIn:
				return Tween<T>::applyCurve<TweenCurve::CubicIn>(t);
			case TweenCurve::CubicOut:
				return Tween<T>::applyCurve<TweenCurve::CubicOut>(t);
			case TweenCurve::Sqrt:
				return Tween<T>::applyCurve<TweenCurve::Sqrt>(t);
			case TweenCurve::SqrtIn:
				return Tween<T>::applyCurve<TweenCurve::SqrtIn>(t);
			case TweenCurve::SqrtOut:
				return Tween<T>::applyCurve<TweenCurve::SqrtOut>(t);
			case TweenCurve::EaseBack:
				return Tween<T>::applyCurve<TweenCurve::EaseBack>(t);
			case TweenCurve::EaseBackIn:
				return Tween<T>::applyCurve<TweenCurve::EaseBackIn>(t);
			case TweenCurve::EaseBackOut:
				return Tween<T>::applyCurve<TweenCurve::EaseBackOut>(t);
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