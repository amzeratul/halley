#include "script_meta.h"

using namespace Halley;

Vector<IGraphNodeType::SettingType> ScriptComment::getSettingTypes() const
{
	return {
		{ "comment", "Halley::String", {} }
	};
}

std::pair<String, Vector<ColourOverride>> ScriptComment::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { "Comment", {} };
}

gsl::span<const IGraphNodeType::PinType> ScriptComment::getPinConfiguration(const ScriptGraphNode& node) const
{
	return {};
}

String ScriptComment::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["comment"].asString("");
}
