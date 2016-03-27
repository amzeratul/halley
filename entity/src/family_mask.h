#pragma once

#include <bitset>

namespace Halley {
	namespace FamilyMask {
		using RealType = std::bitset<128>;


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


		/*
		static RealType make(int index, RealType mask) {
			// TODO
			//return (static_cast<RealType>(1) << static_cast<RealType>(index)) | mask;
			return RealType();
		}
		*/
		inline void setBit(RealType& mask, int bit) {
			mask[bit] = true;
		}

		inline bool hasBit(HandleType handle, int bit) {
			return handle.getRealValue()[bit];
		}



		HandleType getHandle(RealType mask);



		template <typename... Ts>
		struct Evaluator;

		template <>
		struct Evaluator <> {
			static RealType makeMask(RealType startValue) {
				return startValue;
			}
		};

		template <typename T, typename... Ts>
		struct Evaluator <T, Ts...> {
			static void makeMask(RealType& mask) {
				FamilyMask::setBit(mask, T::componentIndex);
				Evaluator<Ts...>::makeMask(mask);
			}

			static HandleType getMask() {
				RealType mask;
				makeMask(mask);
				return getHandle(mask);
			}
		};



		template <typename... Ts>
		struct MutableEvaluator;

		template <>
		struct MutableEvaluator <> {
			static RealType makeMask(RealType startValue) {
				return startValue;
			}
		};

		template <typename T, typename... Ts>
		struct MutableEvaluator <T, Ts...> {
			static void makeMask(RealType& mask) {
				if (std::is_const<T>::value) {
					FamilyMask::setBit(mask, T::componentIndex);
				}
				Evaluator<Ts...>::makeMask(mask);
			}

			static HandleType getMask() {
				RealType mask;
				makeMask(mask);
				return getHandle(mask);
			}
		};
	}

	using FamilyMaskType = FamilyMask::HandleType;
}
