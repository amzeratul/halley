#pragma once

#include <bitset>
#include <optional>
#include "halley/data_structures/hash_map.h"
#include "halley/utils/hash.h"
#include "halley/data_structures/maybe_ref.h"
#include <gsl/span>
#include "halley/support/assert.h"

namespace Halley {
	class HalleyStatics;

	template <typename T>
	using DefaultHash = std::hash<T>;

	constexpr static int maxComponents = 512; // Increasing this number has performance consequences

	namespace FamilyMask {
		using UnderlyingType = std::bitset<maxComponents>;
		struct RealType {
			alignas(64) UnderlyingType v;
		};

		struct MaskEntry
		{
			UnderlyingType mask;
			int idx;

			MaskEntry(const UnderlyingType& m, int i)
				: mask(m)
				, idx(i)
			{
			}

			bool operator==(const MaskEntry& o) const {
				return mask == o.mask;
			}
		};
	}
}

namespace std {
	template<>
	struct hash<Halley::FamilyMask::MaskEntry>
	{
		std::size_t operator()(Halley::FamilyMask::MaskEntry const& s) const noexcept
		{
			static_assert(std::is_trivially_copyable_v<decltype(s.mask)>);
			return Halley::Hash::hash(s.mask);
		}
	};
}

namespace Halley {
	namespace FamilyMask {

		class MaskStorage
		{
		public:
			Vector<RealType> values;
			HashSet<MaskEntry> entries;

			MaskStorage()
			{
				getHandle(RealType());
			}

			int getHandle(const RealType& value)
			{
				const auto i = entries.find(MaskEntry(value.v, 0));
				if (i == entries.end()) [[unlikely]] {
					// Not found, assign a new index
					const int idx = static_cast<int>(values.size());
					auto entry = MaskEntry(value.v, idx);

					// Insert new entry
					entries.insert(entry);
					values.emplace_back(value);

					return idx;
				} else {
					// Found
					return i->idx;
				}
			}

			const RealType& retrieve(int handle) const
			{
				return values[handle];
			}
		};

		class Handle
		{
		public:
			constexpr Handle() = default;
			constexpr Handle(const Handle& h) = default;
			constexpr Handle(Handle&& h) noexcept = default;

			Handle(const RealType& mask, MaskStorage& storage)
				: value(storage.getHandle(mask))
			{
			}

			constexpr Handle& operator=(const Handle& h) { value = h.value; return *this; }

			constexpr bool operator==(const Handle& h) const { return value == h.value; }
			constexpr bool operator!=(const Handle& h) const { return value != h.value; }
			constexpr bool operator<(const Handle& h) const { return value < h.value; }

			const RealType& getRealValue(MaskStorage& storage) const { return storage.retrieve(value); }
			int getRawValue() const { return value; }
			
			Handle intersection(const Handle& h, MaskStorage& storage) const
			{
				return Handle(RealType{ getRealValue(storage).v & h.getRealValue(storage).v }, storage);
			}

			bool contains(const Handle& handle, MaskStorage& storage) const
			{
				const auto& mine = getRealValue(storage);
				const auto& theirs = handle.getRealValue(storage);

				return (mine.v & theirs.v) == theirs.v;
			}

			bool intersects(const Handle& handle, MaskStorage& storage) const
			{
				const auto& mine = getRealValue(storage);
				const auto& theirs = handle.getRealValue(storage);

				return (mine.v & theirs.v).any();
			}

			bool unionChangedBetween(const Handle& a, const Handle& b, MaskStorage& storage) const
			{
				const auto& mine = getRealValue(storage);
				const auto& theirsA = a.getRealValue(storage);
				const auto& theirsB = b.getRealValue(storage);

				return (mine.v & theirsA.v) != (mine.v & theirsB.v);
			}

		private:
			int value = 0;
		};

		using HandleType = Handle;


		inline void setBit(RealType& mask, int bit) {
			HalleyAssertDebug(bit < maxComponents);
			mask.v[bit] = true;
		}

		inline bool hasBit(HandleType handle, int bit, MaskStorage& storage) {
			HalleyAssertDebug(bit < maxComponents);
			return handle.getRealValue(storage).v[bit];
		}

		inline bool hasAnyBit(HandleType handle, gsl::span<const int> bits, MaskStorage& storage) {
			const auto& val = handle.getRealValue(storage);
			for (auto bit: bits) {
				HalleyAssertDebug(bit < maxComponents);
				if (val.v[bit]) {
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
		struct ExclusionEvaluator;

		template <>
		struct ExclusionEvaluator <> {
			static RealType makeMask(RealType startValue) {
				return startValue;
			}

			static std::optional<HandleType> getMask(MaskStorage&) {
				return std::nullopt;
			}
		};

		template <typename T, typename... Ts>
		struct ExclusionEvaluator <T, Ts...> {
			static void makeMask(RealType& mask) {
				FamilyMask::setBit(mask, RetrieveComponentIndex<T>::componentIndex);
				ExclusionEvaluator<Ts...>::makeMask(mask);
			}

			static std::optional<HandleType> getMask(MaskStorage& storage) {
				RealType mask;
				makeMask(mask);
				if (mask.v.any()) {
					return HandleType(mask, storage);
				} else {
					return std::nullopt;
				}
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
	using MaskStorage = FamilyMask::MaskStorage;
}

namespace std {
	template<>
	struct hash<Halley::FamilyMask::Handle>
	{
		std::size_t operator()(const Halley::FamilyMask::Handle& h) const noexcept
		{
			return Halley::DefaultHash<int>()(h.getRawValue());
		}
	};
}
