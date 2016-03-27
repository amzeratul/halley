#pragma once

namespace Halley {
	namespace FamilyMask {
		using RealType = unsigned long long;


		class Handle
		{
		public:
			Handle();
			Handle(const Handle& h);
			Handle(Handle&& h);
			Handle(const RealType& mask);
			Handle(RealType&& mask);

			void operator=(const Handle& h);

			bool operator==(const Handle& h) const;
			bool operator!=(const Handle& h) const;
			bool operator<(const Handle& h) const;
			Handle operator&(const Handle& h) const;

			const RealType& getRealValue() const;

		private:
			int value = -1;
		};

		using HandleType = Handle;



		constexpr static RealType make(int index, RealType mask = 0) {
			return (static_cast<RealType>(1) << static_cast<RealType>(index)) | mask;
		}



		HandleType getHandle(RealType mask);



		template <typename... Ts>
		struct Evaluator;

		template <>
		struct Evaluator <> {
			static constexpr RealType makeMask(RealType startValue) {
				return startValue;
			}
		};

		template <typename T, typename... Ts>
		struct Evaluator <T, Ts...> {
			static constexpr RealType makeMask(RealType startValue) {
				return Evaluator<Ts...>::makeMask(FamilyMask::make(T::componentIndex, startValue));
			}

			static HandleType getMask() {
				return getHandle(makeMask(0));
			}
		};



		template <typename... Ts>
		struct MutableEvaluator;

		template <>
		struct MutableEvaluator <> {
			static constexpr RealType makeMask(RealType startValue) {
				return startValue;
			}
		};

		template <typename T, typename... Ts>
		struct MutableEvaluator <T, Ts...> {
			static constexpr RealType makeMask(RealType startValue) {
				return Evaluator<Ts...>::makeMask(FamilyMask::make(std::is_const<T>::value ? 0 : T::componentIndex, startValue));
			}

			static HandleType getMask() {
				return getHandle(makeMask(0));
			}
		};
	}

	using FamilyMaskType = FamilyMask::HandleType;
}
