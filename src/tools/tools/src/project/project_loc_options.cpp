#include "halley/tools/project/project_loc_options.h"

#include "halley/data_structures/config_node.h"
#include "halley/text/i18n_language.h"

using namespace Halley;

LocalisationFilterRules::LocalisationFilterRules(const ConfigNode& node)
{
	minPriorityForReady = node["minPriorityForReady"].asHashMap<String, LocPriority>();
}

LocPriority LocalisationFilterRules::getMinPriorityForReady(const I18NLanguage& language) const
{
	LocPriority result = LocPriority::Normal;
	int bestMatchLevel = 0;

	for (const auto& [code, priority]: minPriorityForReady) {
		int matchLevel = 0;
		if (code == language.getISOCode()) {
			matchLevel = 2;
		} else if (code == "*") {
			matchLevel = 1;
		}

		if (matchLevel > bestMatchLevel) {
			result = priority;
			bestMatchLevel = matchLevel;
		}
	}

	return result;
}
