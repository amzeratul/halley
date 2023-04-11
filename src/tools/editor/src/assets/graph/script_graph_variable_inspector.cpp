#include "script_graph_variable_inspector.h"

using namespace Halley;

ScriptGraphVariableEntry::ScriptGraphVariableEntry(UIFactory& factory)
	: UIWidget("ScriptGraphVariableEntry", {}, UISizer())
{
	factory.loadUI(*this, "halley/script_variable_entry");
}

void ScriptGraphVariableEntry::onMakeUI()
{
	nameLabel = getWidgetAs<UILabel>("name");
	valueLabel = getWidgetAs<UILabel>("value");
	typeLabel = getWidgetAs<UILabel>("type");
	scopeLabel = getWidgetAs<UILabel>("scope");
}

void ScriptGraphVariableEntry::setData(const String& name, const String& value, const String& type, const String& scope)
{
	nameLabel->setText(LocalisedString::fromHardcodedString(name));
	valueLabel->setText(LocalisedString::fromHardcodedString(value));
	typeLabel->setText(LocalisedString::fromHardcodedString(type));
	scopeLabel->setText(LocalisedString::fromHardcodedString(scope));
}

ScriptGraphVariableInspector::ScriptGraphVariableInspector(UIFactory& factory)
	: UIWidget("ScriptGraphVariableInspector", {}, UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "halley/script_variable_inspector");
}

void ScriptGraphVariableInspector::onMakeUI()
{
	list = getWidget("scrollbar");
	setActive(false);
}

void ScriptGraphVariableInspector::updateVariables(const ConfigNode& data)
{
	list->clear();
	if (data.getType() != ConfigNodeType::Map) {
		setActive(false);
		return;
	}

	const auto& entityVariables = data["entity"];
	const auto& localVariables = data["local"];
	const auto& sharedVariables = data["shared"];

	int count = 0;
	for (const auto& variable : entityVariables.asMap()) {
		const auto entry = std::make_shared<ScriptGraphVariableEntry>(factory);
		entry->setData(variable.first, variable.second.asString(), toString(variable.second.getType()), "Entity");
		list->add(entry);
		count++;
	}

	for (const auto& variable : localVariables.asMap()) {
		const auto entry = std::make_shared<ScriptGraphVariableEntry>(factory);
		entry->setData(variable.first, variable.second.asString(), toString(variable.second.getType()), "Local");
		list->add(entry);
		count++;
	}

	for (const auto& variable : sharedVariables.asMap()) {
		const auto entry = std::make_shared<ScriptGraphVariableEntry>(factory);
		entry->setData(variable.first, variable.second.asString(), toString(variable.second.getType()), "Shared");
		list->add(entry);
		count++;
	}

	setActive(count > 0);
}