#include "family_mask.h"
#include <set>

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

	bool operator<(const MaskEntry& o) const {
		return mask < o.mask;
	}

	bool operator==(const MaskEntry& o) const {
		return mask == o.mask;
	}
};

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
	std::set<MaskEntry> entries;

	static int getHandle(RealType& value)
	{
		auto& instance = getInstance();
		auto entry = MaskEntry(value, 0);
		auto i = instance.entries.find(entry);
		if (i == instance.entries.end()) {
			// Not found
			entry.idx = static_cast<int>(instance.values.size());
			auto result = instance.entries.insert(std::move(entry));
			instance.values.push_back(const_cast<MaskEntry*>(&*result.first));
			return entry.idx;
		} else {
			// Found
			return i->idx;
		}
	}

	static RealType& retrieve(int handle)
	{
		return getInstance().values[handle]->mask;
	}
};


Handle::Handle()
	: value(0)
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
	: value(mask)
{
}

Handle::Handle(RealType&& mask)
	: value(mask)
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
	return Handle(value & h.value);
}

const RealType& Handle::getRealValue() const
{
	return value;
}

HandleType FamilyMask::getHandle(RealType mask)
{
	return Handle(mask);
}
