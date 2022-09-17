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
	ColourStringBuilder str;
	str.append(node.getSettings()["comment"].asString(""), Colour4f(0.36f, 0.55f, 0.41f));
	return str.moveResults();
}

gsl::span<const IGraphNodeType::PinType> ScriptComment::getPinConfiguration(const ScriptGraphNode& node) const
{
	return {};
}
