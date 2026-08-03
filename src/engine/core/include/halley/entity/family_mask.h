#pragma once

#include <bitset>
#include "halley/data_structures/maybe_ref.h"
#include <gsl/span>
#include "halley/support/assert.h"

class MaskStorage;

namespace Halley {
	class HalleyStatics;

	template <typename T>
	using DefaultHash = std::hash<T>;

	constexpr static int maxComponents = 512; // Increasing this number has performance consequences

	namespace FamilyMask {
		using RealType = std::bitset<maxComponents>;


		class Handle
		{
		public:
			constexpr Handle() = default;
			constexpr Handle(const Handle& h) = default;
			constexpr Handle(Handle&& h) noexcept = default;
			Handle(const RealType& mask, MaskStorage& storage);

			constexpr Handle& operator=(const Handle& h) { value = h.value; return *this; }

			constexpr bool operator==(const Handle& h) const { return value == h.value; }
			constexpr bool operator!=(const Handle& h) const { return value != h.value; }
			constexpr bool operator<(const Handle& h) const { return value < h.value; }

			const RealType& getRealValue(MaskStorage& storage) const;
			int getRawValue() const { return value; }
			
			Handle intersection(const Handle& h, MaskStorage& storage) const;
			bool contains(const Handle& handle, MaskStorage& storage) const;
			bool unionChangedBetween(const Handle& a, const Handle& b, MaskStorage& storage) const;

		private:
			int value = -1;
		};

		using HandleType = Handle;


		inline void setBit(RealType& mask, int bit) {
			HalleyAssertDebug(bit < maxComponents);
			mask[bit] = true;
		}

		inline bool hasBit(HandleType handle, int bit, MaskStorage& storage) {
			HalleyAssertDebug(bit < maxComponents);
			return handle.getRealValue(storage)[bit];
		}

		inline bool hasAnyBit(HandleType handle, gsl::span<const int> bits, MaskStorage& storage) {
			const auto& val = handle.getRealValue(storage);
			for (auto bit: bits) {
				HalleyAssertDebug(bit < maxComponents);
				if (val[bit]) {
					return true;
				}
			}
			return false;
		}



		template <typename T>
		struct RetrieveComponentIndex {
			static constexpr int componentIndex = T::componentIndex;
			static constexpr int quickIndex = T::quickIndex;
		};

		template <typename T>
		struct RetrieveComponentIndex<MaybeRef<T>> {
			static constexpr int componentIndex = T::componentIndex;
			static constexpr int quickIndex = T::quickIndex;
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
			static void makeMask(RealType& mask) noexcept {
				FamilyMask::setBit(mask, RetrieveComponentIndex<T>::componentIndex);
				Evaluator<Ts...>::makeMask(mask);
			}

			static HandleType getMask(MaskStorage& storage) {
				RealType mask;
				makeMask(mask);
				return HandleType(mask, storage);
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
			constexpr static void makeMask(RealType& mask) {
				if constexpr (!std::is_const<T>::value) {
					FamilyMask::setBit(mask, RetrieveComponentIndex<T>::componentIndex);
				}
				Evaluator<Ts...>::makeMask(mask);
			}

			constexpr static HandleType getMask(MaskStorage& storage) {
				RealType mask;
				makeMask(mask);
				return Handle(mask, storage);
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
				if constexpr (!IsMaybeRef<T>::value) {
					FamilyMask::setBit(mask, RetrieveComponentIndex<T>::componentIndex);
				}
				InclusionEvaluator<Ts...>::makeMask(mask);
			}

			static HandleType getMask(MaskStorage& storage) {
				RealType mask;
				makeMask(mask);
				return HandleType(mask, storage);
			}
		};


		
		template <typename... Ts>
		struct OptionalEvaluator;

		template <>
		struct OptionalEvaluator <> {
			static RealType makeMask(RealType startValue) {
				return startValue;
			}
		};

		template <typename T, typename... Ts>
		struct OptionalEvaluator <T, Ts...> {
			static void makeMask(RealType& mask) {
				if constexpr (IsMaybeRef<T>::value) {
					FamilyMask::setBit(mask, RetrieveComponentIndex<T>::componentIndex);
				}
				OptionalEvaluator<Ts...>::makeMask(mask);
			}

			static HandleType getMask(MaskStorage& storage) {
				RealType mask;
				makeMask(mask);
				return HandleType(mask, storage);
			}
		};
		

		class MaskStorageInterface {
		public:
			static std::shared_ptr<MaskStorage> createStorage();
		};
	}

	using FamilyMaskType = FamilyMask::HandleType;
}

namespace std {
	//template <class T> struct hash;
	template<> struct hash<Halley::FamilyMask::Handle>
	{
		std::size_t operator()(const Halley::FamilyMask::Handle& h) const noexcept
		{
			return Halley::DefaultHash<int>()(h.getRawValue());
		}
	};
}
