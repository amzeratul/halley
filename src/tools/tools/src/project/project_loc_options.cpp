#include "halley/tools/project/project_loc_options.h"

#include "halley/data_structures/config_node.h"

using namespace Halley;

LocalisationFilterRules::LocalisationFilterRules(const ConfigNode& node)
{
	minPriorityForReady = node["minPriorityForReady"].asEnum(LocPriority::Normal);
}
