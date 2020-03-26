#pragma once

#include <bitset>
#include "halley/data_structures/maybe_ref.h"

class MaskStorage;

namespace Halley {
	class HalleyStatics;

	namespace FamilyMask {
		using RealType = std::bitset<256>;


		class Handle
		{
		public:
			Handle();
			Handle(const Handle& h);
			Handle(Handle&& h) noexcept;
			Handle(const RealType& mask, MaskStorage& storage);

			Handle& operator=(const Handle& h);

			bool operator==(const Handle& h) const;
			bool operator!=(const Handle& h) const;
			bool operator<(const Handle& h) const;

			const RealType& getRealValue(MaskStorage& storage) const;
			
			Handle intersection(const Handle& h, MaskStorage& storage) const;
			bool contains(const Handle& handle, MaskStorage& storage) const;

		private:
			int value = -1;
		};

		using HandleType = Handle;


		inline void setBit(RealType& mask, int bit) {
			mask[bit] = true;
		}

		inline bool hasBit(HandleType handle, int bit, MaskStorage& storage) {
			return handle.getRealValue(storage)[bit];
		}



		HandleType getHandle(RealType mask, MaskStorage& storage);


		template <typename T>
		struct RetrieveComponentIndex {
			static constexpr int componentIndex = T::componentIndex;
		};

		template <typename T>
		struct RetrieveComponentIndex<MaybeRef<T>> {
			static constexpr int componentIndex = T::componentIndex;
		};




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
				FamilyMask::setBit(mask, RetrieveComponentIndex<T>::componentIndex);
				Evaluator<Ts...>::makeMask(mask);
			}

			static HandleType getMask(MaskStorage& storage) {
				RealType mask;
				makeMask(mask);
				return getHandle(mask, storage);
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
					FamilyMask::setBit(mask, RetrieveComponentIndex<T>::componentIndex);
				}
				Evaluator<Ts...>::makeMask(mask);
			}

			static HandleType getMask(MaskStorage& storage) {
				RealType mask;
				makeMask(mask);
				return getHandle(mask, storage);
			}
		};


		template <typename T>
		struct IsMaybeRef : std::false_type {};

		template <typename T>
		struct IsMaybeRef<MaybeRef<T>> : std::true_type {};


		template <typename... Ts>
		struct InclusionEvaluator;

		template <>
		struct InclusionEvaluator <> {
			static RealType makeMask(RealType startValue) {
				return startValue;
			}
		};

		template <typename T, typename... Ts>
		struct InclusionEvaluator <T, Ts...> {
			static void makeMask(RealType& mask) {
				if (!IsMaybeRef<T>::value) {
					FamilyMask::setBit(mask, RetrieveComponentIndex<T>::componentIndex);
				}
				InclusionEvaluator<Ts...>::makeMask(mask);
			}

			static HandleType getMask(MaskStorage& storage) {
				RealType mask;
				makeMask(mask);
				return getHandle(mask, storage);
			}
		};

		class MaskStorageInterface {
		public:
			static std::shared_ptr<MaskStorage> createStorage();
		};
	}

	using FamilyMaskType = FamilyMask::HandleType;
}
