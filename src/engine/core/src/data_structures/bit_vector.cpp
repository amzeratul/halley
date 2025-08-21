#include "halley/data_structures/bit_vector.h"

#include "halley/data_structures/config_node.h"

using namespace Halley;

BitVector::BitVector(const ConfigNode& node)
{
	data = node.asVector<uint8_t>({});
}

ConfigNode BitVector::toConfigNode() const
{
	return ConfigNode(data);
}
