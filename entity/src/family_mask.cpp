#include "family_mask.h"
#include <set>
#include <unordered_set>

using namespace Halley;
using namespace FamilyMask;

struct MaskEntry
{
	RealType mask;
	int idx;

	MaskEntry(MaskEntry&& o)
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
	template <class T> struct hash;
	template<> struct hash<MaskEntry>
	{
		std::size_t operator()(MaskEntry const& s) const {
			return std::hash<RealType>()(s.mask);
		}
	};
}

class MaskStorage
{
public:
	static MaskStorage& getInstance()
	{
		static MaskStorage* i = nullptr;
		if (!i) {
			i = new MaskStorage();
		}
		return *i;
	}

	std::vector<MaskEntry*> values;
	std::unordered_set<MaskEntry> entries;

	static int getHandle(const RealType& value)
	{
		auto& instance = getInstance();
		auto entry = MaskEntry(value, 0);
		auto i = instance.entries.find(entry);
		if (i == instance.entries.end()) {
			// Not found
			int idx = static_cast<int>(instance.values.size());
			entry.idx = idx;
			auto result = instance.entries.insert(std::move(entry));
			instance.values.push_back(const_cast<MaskEntry*>(&*result.first));
			return idx;
		} else {
			// Found
			return i->idx;
		}
	}

	static RealType& retrieve(int handle)
	{
		static RealType dummy;
		if (handle == -1) {
			return dummy;
		} else {
			return getInstance().values[handle]->mask;
		}
	}
};


Handle::Handle()
	: value(-1)
{
}

Handle::Handle(const Handle& h)
	: value(h.value)
{
}

Handle::Handle(Handle&& h)
	: value(h.value)
{
}

Handle::Handle(const RealType& mask)
	: value(MaskStorage::getHandle(mask))
{
}

Handle::Handle(RealType&& mask)
	: value(MaskStorage::getHandle(mask))
{
}

void Handle::operator=(const Handle& h)
{
	value = h.value;
}

bool Handle::operator==(const Handle& h) const
{
	return value == h.value;
}

bool Handle::operator!=(const Handle& h) const
{
	return value != h.value;
}

bool Handle::operator<(const Handle& h) const
{
	return value < h.value;
}

Handle Handle::operator&(const Handle& h) const
{
	return Handle(getRealValue() & h.getRealValue());
}

const RealType& Handle::getRealValue() const
{
	return MaskStorage::retrieve(value);
}

HandleType FamilyMask::getHandle(RealType mask)
{
	return Handle(mask);
}
