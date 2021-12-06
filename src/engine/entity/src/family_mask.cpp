#include "family_mask.h"
#include <unordered_set>
#include <halley/data_structures/vector.h>
#include <functional>
#include "halley/core/game/halley_statics.h"
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
			return std::hash<RealType>()(s.mask);
		}
	};
}

class MaskStorage
{
public:
	Vector<MaskEntry*> values;
	HashSet<MaskEntry> entries;

	int getHandle(const RealType& value)
	{
		auto entry = MaskEntry(value, 0);
		auto i = entries.find(entry);
		if (i == entries.end()) {
			// Not found, assign a new index
			const int idx = static_cast<int>(values.size());
			entry.idx = idx;

			// Insert new entry
			entries.insert(entry);
			values.emplace_back();

			// A re-hash might have happened, so update all references
			for (auto& e: entries) {
				values[e.idx] = &e;
			}

			return idx;
		} else {
			// Found
			return i->idx;
		}
	}

	RealType& retrieve(int handle)
	{
		if (handle == -1) {
			return dummy;
		} else {
			return values[handle]->mask;
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
