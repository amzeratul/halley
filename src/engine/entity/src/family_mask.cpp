#include "family_mask.h"
#include <unordered_set>
#include <halley/data_structures/vector.h>
#include <functional>
#include "halley/core/game/halley_statics.h"

using namespace Halley;
using namespace FamilyMask;

struct MaskEntry
{
	RealType mask;
	int idx;

	MaskEntry(MaskEntry&& o) noexcept
		: mask(std::move(o.mask))
		, idx(o.idx)
	{}

	MaskEntry(const RealType& m, int i)
		: mask(m)
		, idx(i)
	{}

	/*
	bool operator<(const MaskEntry& o) const {
		return mask < o.mask;
	}
	*/

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
	std::unordered_set<MaskEntry> entries;

	int getHandle(const RealType& value)
	{
		auto entry = MaskEntry(value, 0);
		auto i = entries.find(entry);
		if (i == entries.end()) {
			// Not found
			int idx = static_cast<int>(values.size());
			entry.idx = idx;
			auto result = entries.insert(std::move(entry));
			values.push_back(const_cast<MaskEntry*>(&*result.first));
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
	auto& mine = getRealValue(storage);
	auto& theirs = handle.getRealValue(storage);

	return (mine & theirs) == theirs;
}

std::shared_ptr<MaskStorage> MaskStorageInterface::createStorage()
{
	return std::make_shared<MaskStorage>();
}
