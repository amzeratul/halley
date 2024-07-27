#include "script_node_variables.h"

#include "halley/maths/interpolation_curve.h"
#include "halley/maths/ops.h"
#include "halley/maths/tween.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "halley/utils/variable.h"
using namespace Halley;

String ScriptVariable::getLargeLabel(const BaseGraphNode& node) const
{
	return node.getSettings()["variable"].asString("");
}

String ScriptVariable::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return node.getSettings()["scope"].asString("local") + ":" + node.getSettings()["variable"].asString("");
}

gsl::span<const IScriptNodeType::PinType> ScriptVariable::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{
		PinType{ ET::WriteDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::TargetPin, PD::Output }
	};
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptVariable::getSettingTypes() const
{
	return {
		SettingType{ "scope", "Halley::ScriptVariableScope", Vector<String>{"local"} },
		SettingType{ "variable", "Halley::String", Vector<String>{""} }
	};
}

std::pair<String, Vector<ColourOverride>> ScriptVariable::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Variable ");
	str.append(node.getSettings()["scope"].asString("local") + ":" + node.getSettings()["variable"].asString(""), settingColour);
	return str.moveResults();
}

ConfigNode ScriptVariable::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto& vars = environment.getVariables(getScope(node));
	return ConfigNode(vars.getVariable(node.getSettings()["variable"].asString("")));
}

EntityId ScriptVariable::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const
{
	const auto& vars = environment.getVariables(getScope(node));
	const auto& data = vars.getVariable(node.getSettings()["variable"].asString(""));
	if (data.getType() == ConfigNodeType::EntityId || data.getType() == ConfigNodeType::Int || data.getType() == ConfigNodeType::Float) {
		return data.asEntityId();
	} else {
		return {};
	}
}

void ScriptVariable::doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const
{
	const auto scope = getScope(node);
	const auto variable = node.getSettings()["variable"].asString("");

	if (scope != ScriptVariableScope::Local && !environment.hasNetworkAuthorityOver(environment.getCurrentEntityId())) {
		Logger::logError(environment.getCurrentGraph()->getAssetId() + ": Cannot write to Script/Entity Variable \"" + variable + "\", not owned by this client");
		return;
	}

	auto& vars = environment.getVariables(scope);
	vars.setVariable(variable, std::move(data));
}

ConfigNode ScriptVariable::doGetDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node) const
{
	return doGetData(environment, node, 1);
}

ScriptVariableScope ScriptVariable::getScope(const ScriptGraphNode& node) const
{
	return fromString<ScriptVariableScope>(node.getSettings()["scope"].asString("local"));
}



String ScriptEntityVariable::getLargeLabel(const BaseGraphNode& node) const
{
	return node.getSettings()["variable"].asString("");
}

String ScriptEntityVariable::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "entity:" + node.getSettings()["variable"].asString("");
}

gsl::span<const IGraphNodeType::PinType> ScriptEntityVariable::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::WriteDataPin, PD::Input },
		PinType{ ET::TargetPin, PD::Output }
	};
	return data;
}

Vector<IGraphNodeType::SettingType> ScriptEntityVariable::getSettingTypes() const
{
	return {
		SettingType{ "variable", "Halley::String", Vector<String>{""} }
	};
}

std::pair<String, Vector<ColourOverride>> ScriptEntityVariable::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Variable ");
	str.append("entity:" + node.getSettings()["variable"].asString(""), settingColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptEntityVariable::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto& vars = environment.getEntityVariables(readEntityId(environment, node, 0));
	return ConfigNode(vars.getVariable(node.getSettings()["variable"].asString("")));
}

EntityId ScriptEntityVariable::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const
{
	const auto& vars = environment.getEntityVariables(readEntityId(environment, node, 0));
	return vars.getVariable(node.getSettings()["variable"].asString("")).asEntityId({});
}

ConfigNode ScriptEntityVariable::doGetDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node) const
{
	return doGetData(environment, node, 1);
}

void ScriptEntityVariable::doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN,
	ConfigNode data) const
{
	auto e = environment.tryGetEntity(readEntityId(environment, node, 0));
	if (e.isValid()) {
		if (!environment.hasNetworkAuthorityOver(e)) {
			Logger::logError(environment.getCurrentGraph()->getAssetId() + ": Cannot write to Entity Variable \"" + node.getSettings()["variable"].asString("") + "\", not owned by this client");
			return;
		}
		environment.setEntityVariable(e.getEntityId(), node.getSettings()["variable"].asString(""), std::move(data));
	}
}


String ScriptLiteral::getLargeLabel(const BaseGraphNode& node) const
{
	const auto& val = getConfigNode(node);
	const auto& str = val.asString("");
	if (val.getType() == ConfigNodeType::String) {
		return "\"" + str + "\"";
	} else if (val.getType() == ConfigNodeType::Undefined) {
		return "null";
	}
	return str;
}

String ScriptLiteral::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getLargeLabel(node);
}

gsl::span<const IScriptNodeType::PinType> ScriptLiteral::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptLiteral::getSettingTypes() const
{
	return { SettingType{ "value", "Halley::String", Vector<String>{"0"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptLiteral::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto data = getConfigNode(node);

	auto str = ColourStringBuilder(false);
	bool quoting = false;
	if (data.getType() == ConfigNodeType::Int) {
		str.append("Int ");
	} else if (data.getType() == ConfigNodeType::Float) {
		str.append("Float ");
	} else if (data.getType() == ConfigNodeType::Float2) {
		str.append("Vector2f ");
	} else if (data.getType() == ConfigNodeType::Bool) {
		str.append("Bool ");
	} else if (data.getType() == ConfigNodeType::Sequence) {
		str.append("Sequence ");
	} else if (data.getType() != ConfigNodeType::Undefined) {
		str.append("String ");
		str.append("\"", settingColour);
		quoting = true;
	}
	
	str.append(data.asString(), settingColour);
	if (quoting) {
		str.append("\"", settingColour);
	}
	return str.moveResults();
}

ConfigNode ScriptLiteral::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return getConfigNode(node);
}

ConfigNode ScriptLiteral::getConfigNode(const BaseGraphNode& node) const
{
	const auto& origValue = node.getSettings()["value"];
	if (origValue.getType() == ConfigNodeType::Int || origValue.getType() == ConfigNodeType::Float || origValue.getType() == ConfigNodeType::Bool || origValue.getType() == ConfigNodeType::Undefined) {
		return ConfigNode(origValue);
	}

	const auto& value = origValue.asString("");
	if (value == "null") {
		return ConfigNode();
	}

	if (value.startsWith("(") && value.endsWith(")")) { // well this is hacky
		const auto string = value.mid(1, value.length() - 2);
		auto strings = string.split(",");
		for (auto& s: strings) {
			s.trimBoth();
		}
		if (strings.size() == 2 && strings[0].isNumber() && strings[1].isNumber()) {
			return ConfigNode(Vector2f(strings[0].toFloat(), strings[1].toFloat()));
		} else if (strings.size() == 3 && strings[0].isNumber() && strings[1].isNumber() && strings[2].isNumber()) {
			return ConfigNode(Vector3f(strings[0].toFloat(), strings[1].toFloat(), strings[2].toFloat()));
		}
	}

	if (value == "true" || value == "false") {
		return ConfigNode(value == "true");
	}

	if (value.isNumber()) {
		return value.isInteger() ? ConfigNode(value.toInteger()) : ConfigNode(value.toFloat());
	}
	return ConfigNode(value);
}



Vector<IGraphNodeType::SettingType> ScriptVariableTable::getSettingTypes() const
{
	return {
		SettingType{ "variable", "Halley::String", Vector<String>{""} }
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptVariableTable::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{
		PinType{ ET::ReadDataPin, PD::Output }
	};
	return data;
}

String ScriptVariableTable::getLargeLabel(const BaseGraphNode& node) const
{
	const auto split = node.getSettings()["variable"].asString("").split('.');
	return split.empty() ? "" : split.back();
}

String ScriptVariableTable::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "variableTable:" + node.getSettings()["variable"].asString("");
}

std::pair<String, Vector<ColourOverride>> ScriptVariableTable::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Variable ");
	str.append(node.getSettings()["variable"].asString(""), settingColour);
	str.append(" on variable table");
	return str.moveResults();
}

ConfigNode ScriptVariableTable::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto* vt = environment.getVariableTable();
	if (vt) {
		const auto key = node.getSettings()["variable"].asString("");
		return ConfigNode(vt->getRawStorage(key));
	}
	return {};
}



Vector<IGraphNodeType::SettingType> ScriptECSVariable::getSettingTypes() const
{
	return {
		SettingType{ "field", "Halley::ScriptComponentFieldType", Vector<String>{""} }
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptECSVariable::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::WriteDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output }
	};
	return data;
}

String ScriptECSVariable::getLargeLabel(const BaseGraphNode& node) const
{
	const auto type = ScriptComponentFieldType(node.getSettings()["field"]);
	return type.getName();
}

String ScriptECSVariable::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	const auto type = ScriptComponentFieldType(node.getSettings()["field"]);
	return getConnectedNodeName(node, graph, 0) + "." + type.getName();
}

std::pair<String, Vector<ColourOverride>> ScriptECSVariable::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	const auto type = ScriptComponentFieldType(node.getSettings()["field"]);
	auto str = ColourStringBuilder(true);
	str.append("ECS Variable ");
	str.append(type.getName(), settingColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(node, graph, 0), settingColour);
	return str.moveResults();
}

ConfigNode ScriptECSVariable::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	EntityRef entityRef{};
	if (node.getPin(0).hasConnection()) {
		entityRef = environment.getWorld().tryGetEntity(readRawEntityId(environment, node, 0));
	} else {
		entityRef = environment.tryGetEntity(readEntityId(environment, node, 0));
	}

	if (entityRef.isValid()) {
		const auto type = ScriptComponentFieldType(node.getSettings()["field"]);
		const auto& reflector = environment.getWorld().getReflection().getComponentReflector(type.component);
		EntitySerializationContext context;
		context.entityContext = &environment;
		context.resources = &environment.getResources();
		context.entitySerializationTypeMask = EntitySerialization::makeMask(EntitySerialization::Type::Dynamic);
		return reflector.serializeField(context, entityRef, type.field);
	}
	return {};
}

void ScriptECSVariable::doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const
{
	EntityRef entityRef{};
	if (node.getPin(0).hasConnection()) {
		entityRef = environment.getWorld().tryGetEntity(readRawEntityId(environment, node, 0));
	}
	else {
		entityRef = environment.tryGetEntity(readEntityId(environment, node, 0));
	}

	if (entityRef.isValid()) {
		const auto type = ScriptComponentFieldType(node.getSettings()["field"]);

		if (!environment.hasNetworkAuthorityOver(entityRef)) {
			Logger::logError(environment.getCurrentGraph()->getAssetId() + ": Cannot write to ECS Variable \"" + type.getName() + "\", not owned by this client");
			return;
		}

		const auto& reflector = environment.getWorld().getReflection().getComponentReflector(type.component);
		EntitySerializationContext context;
		context.entityContext = &environment;
		context.resources = &environment.getResources();
		context.entitySerializationTypeMask = EntitySerialization::makeMask(EntitySerialization::Type::Dynamic);
		reflector.deserializeField(context, entityRef, type.field, data);
	}
}


String ScriptColourLiteral::getLargeLabel(const BaseGraphNode& node) const
{
	return node.getSettings()["value"].asString("#FFFFFF");
}

String ScriptColourLiteral::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getLargeLabel(node);
}

gsl::span<const IScriptNodeType::PinType> ScriptColourLiteral::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptColourLiteral::getSettingTypes() const
{
	return { SettingType{ "value", "Halley::Colour4f", Vector<String>{"#FFFFFF"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptColourLiteral::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(false);
	str.append("Colour");
	return str.moveResults();
}

ConfigNode ScriptColourLiteral::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return ConfigNode(node.getSettings()["value"].asString("#FFFFFF"));
}



String ScriptComparison::getLargeLabel(const BaseGraphNode& node) const
{
	return node.getSettings()["operator"].asString("==");
}

String ScriptComparison::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	auto b = getConnectedNodeName(node, graph, 1);
	auto op = getLargeLabel(node);
	return addParentheses(std::move(a)) + " " + op + " " + addParentheses(std::move(b));
}

gsl::span<const IScriptNodeType::PinType> ScriptComparison::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptComparison::getSettingTypes() const
{
	return { SettingType{ "operator", "Halley::MathRelOp", Vector<String>{"=="} } };
}

std::pair<String, Vector<ColourOverride>> ScriptComparison::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("True if ");
	str.append(addParentheses(getConnectedNodeName(node, graph, 0)), parameterColour);
	str.append(" ");
	str.append(node.getSettings()["operator"].asString("=="), settingColour);
	str.append(" ");
	str.append(addParentheses(getConnectedNodeName(node, graph, 1)), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptComparison::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto a = readDataPin(environment, node, 0);
	const auto b = readDataPin(environment, node, 1);
	const auto op = fromString<MathRelOp>(node.getSettings()["operator"].asString("=="));
	return ConfigNode(a.compareTo(op, b));
}



String ScriptArithmetic::getLargeLabel(const BaseGraphNode& node) const
{
	return node.getSettings()["operator"].asString("+");
}

String ScriptArithmetic::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	auto b = getConnectedNodeName(node, graph, 1);
	auto op = getLargeLabel(node);

	if (op[0] >= 'a' && op[0] <= 'z') {
		// If starts with a lower case letter, assume it's in function form
		return op + "(" + a + ", " + b + ")";
	} else if (a == "<empty>" && op == "-") {
		// Prefix form
		return op + addParentheses(std::move(b));
	} else {
		// Otherwise, assume infix form
		return addParentheses(std::move(a)) + " " + op + " " + addParentheses(std::move(b));
	}
}

gsl::span<const IScriptNodeType::PinType> ScriptArithmetic::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptArithmetic::getSettingTypes() const
{
	return { SettingType{ "operator", "Halley::MathOp", Vector<String>{"+"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptArithmetic::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Returns ");
	str.append(getShortDescription(dynamic_cast<const ScriptGraphNode&>(node), dynamic_cast<const ScriptGraph&>(graph), 0), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptArithmetic::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pin_n) const
{
	const auto a = readDataPin(environment, node, 0);
	const auto b = readDataPin(environment, node, 1);
	const auto op = fromString<MathOp>(node.getSettings()["operator"].asString("+"));

	const auto type = ConfigNode::getPromotedType(std::array<ConfigNodeType, 2>{ a.getType(), b.getType() }, true);

	if (type == ConfigNodeType::String) {
		if (op == MathOp::Add) {
			if (Colour4f::isColour(a.asString("")) && Colour4f::isColour(b.asString(""))) {
				// Treat as colours
				return (Colour4f(a) + Colour4f(b)).toConfigNode();
			}

			return ConfigNode(a.asString("") + b.asString(""));
		} else {
			Logger::logError("Attempting to perform illegal op " + toString(op) + " with string types.");
			return ConfigNode();
		}
	} else if (type == ConfigNodeType::Float) {
		return ConfigNode(MathOps::apply(op, a.asFloat(0), b.asFloat(0)));
	} else if (type == ConfigNodeType::Int64) {
		return ConfigNode(MathOps::apply(op, a.asInt64(0), b.asInt64(0)));
	} else if (type == ConfigNodeType::Int) {
		return ConfigNode(MathOps::apply(op, a.asInt(0), b.asInt(0)));
	} else if (type == ConfigNodeType::Float2) {
		return ConfigNode(MathOps::apply(op, a.asVector2f({}), b.asVector2f({})));
	} else if (type == ConfigNodeType::Int2) {
		return ConfigNode(MathOps::apply(op, a.asVector2i({}), b.asVector2i({})));
	} else {
		Logger::logError("ScriptArithmetic node can't perform arithmetic with types " + toString(a.getType()) + " and " + toString(b.getType()));
		return ConfigNode();
	}
}



String ScriptValueOr::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	auto b = getConnectedNodeName(node, graph, 1);
	return addParentheses(std::move(a)) + " ?? " + addParentheses(std::move(b));
}

gsl::span<const IScriptNodeType::PinType> ScriptValueOr::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptValueOr::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Returns ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	str.append(" if defined, otherwise ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptValueOr::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto a = readDataPin(environment, node, 0);
	if (a.getType() != ConfigNodeType::Undefined) {
		return a;
	} else {
		return readDataPin(environment, node, 1);
	}
}



String ScriptConditionalOperator::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	auto b = getConnectedNodeName(node, graph, 1);
	auto c = getConnectedNodeName(node, graph, 2);
	return addParentheses(std::move(a)) + " ? " + addParentheses(std::move(b)) + " : " + addParentheses(std::move(c));
}

gsl::span<const IGraphNodeType::PinType> ScriptConditionalOperator::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptConditionalOperator::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("If ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	str.append(", ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	str.append(", otherwise ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptConditionalOperator::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto a = readDataPin(environment, node, 0);
	if (a.asBool(false)) {
		return readDataPin(environment, node, 1);
	} else {
		return readDataPin(environment, node, 2);
	}
}


Vector<IScriptNodeType::SettingType> ScriptLerp::getSettingTypes() const
{
	return {
		SettingType{ "from", "float", Vector<String>{"0"} },
		SettingType{ "to", "float", Vector<String>{"1"} },
		SettingType{ "curve", "Halley::InterpolationCurveLerp", Vector<String>{"linear"} }
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptLerp::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{
	    PinType{ ET::ReadDataPin, PD::Input },
	    PinType{ ET::ReadDataPin, PD::Output },
	    PinType{ ET::ReadDataPin, PD::Input },
	    PinType{ ET::ReadDataPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLerp::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Returns lerp(");

	if (node.getPin(2).hasConnection()) {
		str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	} else {
		str.append(toString(node.getSettings()["from"].asFloat(0)), settingColour);
	}

	str.append(", ");

	if (node.getPin(3).hasConnection()) {
		str.append(getConnectedNodeName(node, graph, 3), parameterColour);
	}
	else {
		str.append(toString(node.getSettings()["to"].asFloat(1)), settingColour);
	}

	str.append(", ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	str.append(")");
	return str.moveResults();
}

String ScriptLerp::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId element_idx) const
{
	if (node.getPin(2).hasConnection() && node.getPin(3).hasConnection()) {
		return "lerp(" + getConnectedNodeName(node, graph, 2) + ", " + getConnectedNodeName(node, graph, 3) + ", " + getConnectedNodeName(node, graph, 0) + ")";
	}

	const auto from = node.getSettings()["from"].asFloat(0);
	const auto to = node.getSettings()["to"].asFloat(1);
	return "lerp(" + toString(from) + ", " + toString(to) + ", " + getConnectedNodeName(node, graph, 0) + ")";
}

String ScriptLerp::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
    switch(elementIdx) {
    case 0:
		return "t";
	case 1:
		return "output";
	case 2:
		return "from";
	case 3:
		return "to";
    }

	return "";
}

ConfigNode ScriptLerp::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pin_n) const
{
	const auto curve = InterpolationCurve(node.getSettings()["curve"], true);
	const auto t = readDataPin(environment, node, 0).asFloat(0);
	const auto factor = curve.evaluate(clamp(t, 0.0f, 1.0f));

	if (node.getPin(2).hasConnection() && node.getPin(3).hasConnection()) {
		const auto from = readDataPin(environment, node, 2);
		const auto to = readDataPin(environment, node, 3);

		if (from.getType() != to.getType()) {
			Logger::logError("Trying to lerp between 2 different types! " + toString(from.getType()) + " " + toString(to.getType()));
		    return {};
		}

		switch(from.getType()) {
		case ConfigNodeType::Float:
			return ConfigNode(lerp(from.asFloat(), to.asFloat(), factor));
		case ConfigNodeType::Float2:
			return ConfigNode(lerp(from.asVector2f(), to.asVector2f(), factor));
		case ConfigNodeType::Int:
			return ConfigNode(lerp(from.asInt(), to.asInt(), factor));
		case ConfigNodeType::String:
			return lerp(Colour4f(from.asString()), Colour4f(to.asString()), factor).toConfigNode();
		default:
			Logger::logError("Trying to lerp an unsupported type! " + toString(from.getType()));
			return {};
		}
	}

	const auto from = node.getSettings()["from"].asFloat(0);
	const auto to = node.getSettings()["to"].asFloat(1);
	return ConfigNode(lerp(from, to, factor));
}



gsl::span<const IScriptNodeType::PinType> ScriptAdvanceTo::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 9>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::WriteDataPin, PD::Output }
	};
	return data;
}

String ScriptAdvanceTo::getPinDescription(const BaseGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Flow output.";
	} else if (elementIdx == 2) {
		return "Flow output if target is reached.";
	} else if (elementIdx == 3) {
		return "Flow output if target is not reached.";
	} else if (elementIdx == 4) {
		return "Flow output if variable was modified.";
	} else if (elementIdx == 5) {
		return "Flow output if variable was not modified.";
	} else if (elementIdx == 6) {
		return "Target value";
	} else if (elementIdx == 7) {
		return "Increment value";
	} else if (elementIdx == 8) {
		return "Variable being modified";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, element, elementIdx);
}

std::pair<String, Vector<ColourOverride>> ScriptAdvanceTo::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Advance ");
	str.append(getConnectedNodeName(node, graph, 8), parameterColour);
	str.append(" towards ");
	str.append(getConnectedNodeName(node, graph, 6), parameterColour);
	str.append(" by ");
	str.append(getConnectedNodeName(node, graph, 7), parameterColour);
	return str.moveResults();
}

namespace {
	template <typename T>
	std::tuple<T, bool, bool> advanceWithFlags(T a, T b, T inc)
	{
		const bool modified = !(a == b);
		const T c = advance<T>(a, b, inc);
		const bool reached = c == b;
		return { c, modified, reached };
	}
}

IScriptNodeType::Result ScriptAdvanceTo::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto target = readDataPin(environment, node, 6);
	const auto amount = readDataPin(environment, node, 7);
	const auto var = readDataPin(environment, node, 8);

	const auto type = ConfigNode::getPromotedType(std::array<ConfigNodeType, 3>{ var.getType(), target.getType(), amount.getType() }, true);
	bool modified = false;
	bool reached = false;
	
	if (type == ConfigNodeType::Float) {
		float v;
		std::tie(v, modified, reached) = advanceWithFlags(var.asFloat(0), target.asFloat(0), amount.asFloat(0));
		//Logger::logDev("AdvanceTo(" + toString(node.getId()) + "): " + toString(var.asFloat(0)) + " -> " + toString(v) + " | modified=" + toString(modified) + ", reached=" + toString(reached));
		writeDataPin(environment, node, 8, ConfigNode(v));
	} else if (type == ConfigNodeType::Int) {
		int v;
		std::tie(v, modified, reached) = advanceWithFlags(var.asInt(0), target.asInt(0), amount.asInt(0));
		writeDataPin(environment, node, 8, ConfigNode(v));
	} else if (type == ConfigNodeType::Int64) {
		int64_t v;
		std::tie(v, modified, reached) = advanceWithFlags(var.asInt64(0), target.asInt64(0), amount.asInt64(0));
		writeDataPin(environment, node, 8, ConfigNode(v));
	} else {
		Logger::logError("Cannot advance variable of type " + toString(type));
	}

	constexpr uint8_t normalOutPinMask = 1 << 0;
	constexpr uint8_t reachedOutPinMask = 1 << 1;
	constexpr uint8_t notReachedOutPinMask = 1 << 2;
	constexpr uint8_t modifiedOutPinMask = 1 << 3;
	constexpr uint8_t notModifiedOutPinMask = 1 << 4;
	return Result(ScriptNodeExecutionState::Done, 0, (normalOutPinMask) | (reached ? reachedOutPinMask : notReachedOutPinMask) | (modified ? modifiedOutPinMask : notModifiedOutPinMask));
}



String ScriptSetVariable::getLabel(const BaseGraphNode& node) const
{
	if (!node.getPin(2).hasConnection()) {
		return toString(node.getSettings()["defaultValue"].asFloat(0));
	}
	return "";
}

gsl::span<const IScriptNodeType::PinType> ScriptSetVariable::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::WriteDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptSetVariable::getSettingTypes() const
{
	return { SettingType{ "defaultValue", "float", Vector<String>{"0"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptSetVariable::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append(getConnectedNodeName(node, graph, 3), parameterColour);
	str.append(" := ");
	str.append(node.getPin(2).hasConnection() ? getConnectedNodeName(node, graph, 2) : getLabel(node), node.getPin(2).hasConnection() ? parameterColour : settingColour);
	return str.moveResults();
}

String ScriptSetVariable::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	if (elementIdx == 3) {
		return node.getPin(2).hasConnection() ? getConnectedNodeName(node, graph, 2) : getLabel(node);
	}
	return "Set Variable";
}

IScriptNodeType::Result ScriptSetVariable::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto data = node.getPin(2).hasConnection() ? readDataPin(environment, node, 2) : ConfigNode(node.getSettings()["defaultValue"].asFloat(0));
	writeDataPin(environment, node, 3, std::move(data));
	return Result(ScriptNodeExecutionState::Done);
}




ScriptHoldVariableData::ScriptHoldVariableData(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		prevValue = ConfigNode(node["prevValue"]);
		started = node["started"].asBool(false);
	} else {
		prevValue = ConfigNode();
		started = false;
	}
}

ConfigNode ScriptHoldVariableData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["prevValue"] = ConfigNode(prevValue);
	result["started"] = started;
	return result;
}

String ScriptHoldVariable::getLabel(const BaseGraphNode& node) const
{
	if (!node.getPin(2).hasConnection()) {
		if (!node.getPin(3).hasConnection()) {
			return toString(node.getSettings()["defaultValue"].asInt(0)) + " -> " + toString(node.getSettings()["defaultPrevValue"].asInt(0));
		} else {
			return toString(node.getSettings()["defaultValue"].asInt(0));
		}
	}
	return "";
}

gsl::span<const IScriptNodeType::PinType> ScriptHoldVariable::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 5>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::WriteDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptHoldVariable::getSettingTypes() const
{
	return {
		SettingType{ "defaultValue", "int", Vector<String>{"0"} },
		SettingType{ "defaultPrevValue", "int", Vector<String>{"0"} },
		SettingType{ "continuous", "bool", Vector<String>{"false"} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptHoldVariable::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	const auto label1 = !node.getPin(2).hasConnection() ? toString(node.getSettings()["defaultValue"].asInt(0)) : "";
	const auto label2 = !node.getPin(3).hasConnection() ? toString(node.getSettings()["defaultPrevValue"].asInt(0)) : "";

	auto str = ColourStringBuilder(true);
	str.append(getConnectedNodeName(node, graph, 4), parameterColour);
	str.append(" := ");
	str.append(label1.isEmpty() ? getConnectedNodeName(node, graph, 2) : label1, label1.isEmpty() ? parameterColour : settingColour);
	str.append(", then ");
	str.append(getConnectedNodeName(node, graph, 4), parameterColour);
	str.append(" := ");
	str.append(label2.isEmpty() ? getConnectedNodeName(node, graph, 3) : label2, label2.isEmpty() ? parameterColour : settingColour);
	if (node.getSettings()["continuous"].asBool(false)) {
		str.append(" (continuously)", settingColour);
	}
	return str.moveResults();
}

void ScriptHoldVariable::doInitData(ScriptHoldVariableData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data = ScriptHoldVariableData(nodeData);
	data.started = false;
}

IScriptNodeType::Result ScriptHoldVariable::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node,ScriptHoldVariableData& curData) const
{
	const bool continuous = node.getSettings()["continuous"].asBool(false);
	const bool first = !curData.started;
	curData.started = true;

	if (first) {
		auto prevData = node.getPin(3).hasConnection() ? readDataPin(environment, node, 3) : ConfigNode(node.getSettings()["defaultPrevValue"].asInt(0));
		curData.prevValue = std::move(prevData);
	}

	auto data = node.getPin(2).hasConnection() ? readDataPin(environment, node, 2) : ConfigNode(node.getSettings()["defaultValue"].asInt(0));
	writeDataPin(environment, node, 4, std::move(data));

	if (continuous) {
		return Result(first ? ScriptNodeExecutionState::Fork : ScriptNodeExecutionState::Executing, first ? 0 : time);
	} else {
		return Result(ScriptNodeExecutionState::Done);
	}
}

void ScriptHoldVariable::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptHoldVariableData& curData) const
{
	writeDataPin(environment, node, 4, std::move(curData.prevValue));
}



Vector<IGraphNodeType::SettingType> ScriptEntityIdToData::getSettingTypes() const
{
	return {
		SettingType{ "readRaw", "bool", Vector<String>{"true"} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptEntityIdToData::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptEntityIdToData::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getConnectedNodeName(node, graph, 0);
}

std::pair<String, Vector<ColourOverride>> ScriptEntityIdToData::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	if (node.getSettings()["readRaw"].asBool(true)) {
		return { "Convert EntityId to data. Using readRaw function.", {} };
	}
	return { "Convert EntityId to data. Using read function which returns entity to which the script is attached to, if input is invalid.", {} };
}

ConfigNode ScriptEntityIdToData::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	if (node.getSettings()["readRaw"].asBool(true)) {
		return ConfigNode(readRawEntityId(environment, node, 0));
	}
	return ConfigNode(readEntityId(environment, node, 0));
}



gsl::span<const IScriptNodeType::PinType> ScriptDataToEntityId::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::TargetPin, PD::Output } };
	return data;
}

String ScriptDataToEntityId::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getConnectedNodeName(node, graph, 0);
}

std::pair<String, Vector<ColourOverride>> ScriptDataToEntityId::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	return { "Convert data to EntityId.", {} };
}

EntityId ScriptDataToEntityId::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const
{
	return readDataPin(environment, node, 0).asEntityId({});
}


gsl::span<const IGraphNodeType::PinType> ScriptToVector::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptToVector::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "(" + getConnectedNodeName(node, graph, 0) + ", " + getConnectedNodeName(node, graph, 1) + ")";
}

std::pair<String, Vector<ColourOverride>> ScriptToVector::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Return (");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	str.append(", ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	str.append(")");
	return str.moveResults();
}

String ScriptToVector::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "x";
	} else if (elementIdx == 1) {
		return "y";
	} else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

ConfigNode ScriptToVector::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto x = readDataPin(environment, node, 0).asFloat(0);
	const auto y = readDataPin(environment, node, 1).asFloat(0);
	return ConfigNode(Vector2f(x, y));
}



gsl::span<const IGraphNodeType::PinType> ScriptFromVector::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptFromVector::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return getConnectedNodeName(node, graph, 0) + ".x";
	} else if (elementIdx == 2) {
		return getConnectedNodeName(node, graph, 0) + ".y";
	}
	return "";
}

std::pair<String, Vector<ColourOverride>> ScriptFromVector::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Split vector ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptFromVector::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "x";
	} else if (elementIdx == 2) {
		return "y";
	} else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

ConfigNode ScriptFromVector::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	if (pinN == 1) {
		return ConfigNode(readDataPin(environment, node, 0).asVector2f({}).x);
	} else if (pinN == 2) {
		return ConfigNode(readDataPin(environment, node, 0).asVector2f({}).y);
	} else {
		return {};
	}
}



gsl::span<const IGraphNodeType::PinType> ScriptInsertValueIntoMap::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptInsertValueIntoMap::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto startMap = getConnectedNodeName(node, graph, 0);
	if (startMap.startsWith("{") && startMap.endsWith("}")) {
		startMap = startMap.mid(1, startMap.size() - 2).trimBoth();
	}
	if (startMap == "<empty>") {
		startMap = {};
	}
	return "{ " + (startMap.isEmpty() ? "" : startMap + ", ") + getConnectedNodeName(node, graph, 2) + " := " + getConnectedNodeName(node, graph, 1) + " }";
}

std::pair<String, Vector<ColourOverride>> ScriptInsertValueIntoMap::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Insert into map ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	str.append(" key ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	str.append(" with value ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	return str.moveResults();
}

String ScriptInsertValueIntoMap::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "Map (can be empty)";
	}
	else if (elementIdx == 1) {
		return "Value";
	}
	else if (elementIdx == 2) {
		return "Key";
	}
	else if (elementIdx == 3) {
		return "Map";
	}
	else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

ConfigNode ScriptInsertValueIntoMap::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto map = readDataPin(environment, node, 0);
	if (map.getType() != ConfigNodeType::Map) {
		map = ConfigNode::MapType();
	}

	const auto value = readDataPin(environment, node, 1);
	const auto key = readDataPin(environment, node, 2).asString("");
	if (key.isEmpty() || value.getType() == ConfigNodeType::Undefined) {
		return map;
	}
	map[key] = value;

    return map;
}



gsl::span<const IGraphNodeType::PinType> ScriptGetValueFromMap::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output }
	};
	return data;
}

String ScriptGetValueFromMap::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getConnectedNodeName(node, graph, 0) + "[" + getConnectedNodeName(node, graph, 1) + "]";
}

std::pair<String, Vector<ColourOverride>> ScriptGetValueFromMap::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Get value with key ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	str.append(" from map ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptGetValueFromMap::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "Map";
	}
	else if (elementIdx == 1) {
		return "Key";
	}
	else if (elementIdx == 2) {
		return "Value";
	}
	else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

ConfigNode ScriptGetValueFromMap::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto map = readDataPin(environment, node, 0);
	if (map.getType() != ConfigNodeType::Map) {
		return {};
	}

	const auto key = readDataPin(environment, node, 1).asString("");
	if (key.isEmpty()) {
		return {};
	}

	if (map.hasKey(key)) {
		return ConfigNode(map[key]);
	} else {
		return {};
	}
}



Vector<IGraphNodeType::SettingType> ScriptPackMap::getSettingTypes() const
{
	return {
		SettingType{ "keys", "Halley::Vector<Halley::String>", Vector<String>{} }
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptPackMap::getPinConfiguration(const BaseGraphNode& node) const
{
	auto keys = node.getSettings()["keys"].asVector<String>({});

	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;

	static Vector<IGraphNodeType::PinType> result;
	result.clear();
	result.push_back(PinType{ ET::ReadDataPin, PD::Output });
	for (const auto& key : keys) {
		result.push_back(PinType{ ET::ReadDataPin, PD::Input });
	}
	return result.span();
}

String ScriptPackMap::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	Vector<String> params;
	auto keys = node.getSettings()["keys"].asVector<String>({});
	for (size_t i = 0; i < keys.size(); ++i) {
		params.push_back(keys[i] + " = " + getConnectedNodeName(node, graph, i + 1));
	}

	return "{ " + String::concatList(params, ", ") + " }";
}

std::pair<String, Vector<ColourOverride>> ScriptPackMap::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	Vector<String> params;
	auto keys = node.getSettings()["keys"].asVector<String>({});
	for (size_t i = 0; i < keys.size(); ++i) {
		params.push_back(keys[i] + " = " + getConnectedNodeName(node, graph, i + 1));
	}

	auto str = ColourStringBuilder();
	str.append("Pack map ");
	str.append("{ " + String::concatList(params, ", ") + " }", parameterColour);
	return str.moveResults();
}

String ScriptPackMap::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "Map Output";
	} else {
		auto keys = node.getSettings()["keys"].asVector<String>({});
		return keys.at(elementIdx - 1);
	}
}

ConfigNode ScriptPackMap::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto keys = node.getSettings()["keys"].asVector<String>({});
	ConfigNode::MapType result;

	for (size_t i = 0; i < keys.size(); ++i) {
		result[keys[i]] = readDataPin(environment, node, i + 1);
	}

	return result;
}


Vector<IGraphNodeType::SettingType> ScriptUnpackMap::getSettingTypes() const
{
	return {
		SettingType{ "keys", "Halley::Vector<Halley::String>", Vector<String>{} }
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptUnpackMap::getPinConfiguration(const BaseGraphNode& node) const
{
	auto keys = node.getSettings()["keys"].asVector<String>({});

	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;

	static Vector<IGraphNodeType::PinType> result;
	result.clear();
	result.push_back(PinType{ ET::ReadDataPin, PD::Input });
	for (const auto& key : keys) {
		result.push_back(PinType{ ET::ReadDataPin, PD::Output });
	}
	return result.span();
}

String ScriptUnpackMap::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto keys = node.getSettings()["keys"].asVector<String>({});
	return getConnectedNodeName(node, graph, 0) + "[\"" + keys.at(elementIdx - 1) + "\"]";
}

std::pair<String, Vector<ColourOverride>> ScriptUnpackMap::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder();
	str.append("Unpack map ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptUnpackMap::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "Map Output";
	} else {
		auto keys = node.getSettings()["keys"].asVector<String>({});
		return keys.at(elementIdx - 1);
	}
}

ConfigNode ScriptUnpackMap::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto keys = node.getSettings()["keys"].asVector<String>({});
	
	auto config = readDataPin(environment, node, 0);
	if (config.getType() == ConfigNodeType::Map) {
		auto& map = config.asMap();
		const auto iter = map.find(keys.at(pinN - 1));
		if (iter != map.end()) {
			return ConfigNode(iter->second);
		} else {
			return {};
		}
	} else {
		return {};
	}
}


gsl::span<const IGraphNodeType::PinType> ScriptInsertValueIntoSequence::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptInsertValueIntoSequence::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto startSeq = getConnectedNodeName(node, graph, 0);
	if (startSeq.startsWith("[") && startSeq.endsWith("]")) {
		startSeq = startSeq.mid(1, startSeq.size() - 2).trimBoth();
	}
	if (startSeq == "<empty>") {
		startSeq = {};
	}
	return "[ " + (startSeq.isEmpty() ? "" : startSeq + ", ") + getConnectedNodeName(node, graph, 1) + " ]";
}

std::pair<String, Vector<ColourOverride>> ScriptInsertValueIntoSequence::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Insert value ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	str.append(" into sequence ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptInsertValueIntoSequence::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "Sequence (Can be empty)";
	}
	else if (elementIdx == 1) {
		return "Value";
	}
	else if (elementIdx == 2) {
		return "Sequence";
	}
	else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

ConfigNode ScriptInsertValueIntoSequence::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto sequence = readDataPin(environment, node, 0);
	const auto value = readDataPin(environment, node, 1);
	if (value.getType() == ConfigNodeType::Undefined) {
		return sequence;
	}

	if (sequence.getType() != ConfigNodeType::Sequence) {
		auto newSequence = ConfigNode::SequenceType();
		newSequence.push_back(value);
		return newSequence;
	}
	sequence.asSequence().push_back(value);

	return ConfigNode(sequence);
}



gsl::span<const IGraphNodeType::PinType> ScriptHasSequenceValue::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptHasSequenceValue::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "Has sequence " +
		getConnectedNodeName(node, graph, 0) +
		" value " +
		getConnectedNodeName(node, graph, 1);
}

std::pair<String, Vector<ColourOverride>> ScriptHasSequenceValue::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Has sequence ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	str.append(" value ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	return str.moveResults();
}

String ScriptHasSequenceValue::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "Sequence";
	}
	else if (elementIdx == 1) {
		return "Value";
	}
	else if (elementIdx == 2) {
		return "Bool";
	}
	else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

ConfigNode ScriptHasSequenceValue::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto sequence = readDataPin(environment, node, 0);
	if (sequence.getType() != ConfigNodeType::Sequence) {
		return {};
	}

	const auto value = readDataPin(environment, node, 1);
	if (value.getType() == ConfigNodeType::Undefined) {
		return {};
	}

	const auto hasValueIter = std_ex::find(sequence.asSequence(), value);
	return ConfigNode(hasValueIter != sequence.asSequence().end());
}
