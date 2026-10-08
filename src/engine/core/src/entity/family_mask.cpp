#include "halley/entity/family_mask.h"
#include <unordered_set>
#include <halley/data_structures/vector.h>
#include <functional>
#include "halley/game/halley_statics.h"
#include "halley/data_structures/hash_map.h"

using namespace Halley;

std::shared_ptr<FamilyMask::MaskStorage> FamilyMask::MaskStorageInterface::createStorage()
{
	return std::make_shared<MaskStorage>();
}
