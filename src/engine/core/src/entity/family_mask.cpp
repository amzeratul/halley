#include "halley/entity/family_mask.h"
#include <unordered_set>
#include <halley/data_structures/vector.h>
#include <functional>
#include "halley/game/halley_statics.h"
#include "halley/data_structures/hash_map.h"

using namespace Halley;
using namespace FamilyMask;

struct MaskEntry
{
	RealType mask;
	int idx;
	
	MaskEntry(const RealType& m, int i)
		: mask(m)
		, idx(i)
	{}

	bool operator==(const MaskEntry& o) const {
		return mask == o.mask;
	}
};

namespace std {
	//template <class T> struct hash;
	template<> struct hash<MaskEntry>
	{
		std::size_t operator()(MaskEntry const& s) const noexcept
		{
			static_assert(std::is_trivially_copyable_v<decltype(s.mask)>);
			return Hash::hash(s.mask);
		}
	};
}

class MaskStorage
{
public:
	Vector<RealType> values;
	HashSet<MaskEntry> entries;

	int getHandle(const RealType& value)
	{
		const auto i = entries.find(MaskEntry(value, -1));
		if (i == entries.end()) [[unlikely]] {
			// Not found, assign a new index
			const int idx = static_cast<int>(values.size());
			auto entry = MaskEntry(value, idx);

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
		if (handle == -1) [[unlikely]] {
			return dummy;
		} else {
			return values[handle];
		}
	}

private:
	RealType dummy;
};


Handle::Handle(const RealType& mask, MaskStorage& storage)
	: value(storage.getHandle(mask))
{
}

Handle Handle::intersection(const Handle& h, MaskStorage& storage) const
{
	return Handle(getRealValue(storage) & h.getRealValue(storage), storage);
}

const RealType& Handle::getRealValue(MaskStorage& storage) const
{
	return storage.retrieve(value);
}

bool Handle::contains(const Handle& handle, MaskStorage& storage) const
{
	const auto& mine = getRealValue(storage);
	const auto& theirs = handle.getRealValue(storage);

	return (mine & theirs) == theirs;
}

bool Handle::intersects(const Handle& handle, MaskStorage& storage) const
{
	const auto& mine = getRealValue(storage);
	const auto& theirs = handle.getRealValue(storage);

	return (mine & theirs).any();
}

bool Handle::unionChangedBetween(const Handle& a, const Handle& b, MaskStorage& storage) const
{
	const auto& mine = getRealValue(storage);
	const auto& theirsA = a.getRealValue(storage);
	const auto& theirsB = b.getRealValue(storage);

	return (mine & theirsA) != (mine & theirsB);
}

std::shared_ptr<MaskStorage> MaskStorageInterface::createStorage()
{
	return std::make_shared<MaskStorage>();
}
