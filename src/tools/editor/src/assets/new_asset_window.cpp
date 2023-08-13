#include "new_asset_window.h"
using namespace Halley;

NewAssetWindow::NewAssetWindow(UIFactory& factory, LocalisedString label, String startValue, String extension, Callback callback)
	: PopupWindow("new_asset")
	, factory(factory)
	, label(std::move(label))
	, startValue(std::move(startValue))
	, extension(std::move(extension))
	, callback(std::move(callback))
{
	makeUI();
}

void NewAssetWindow::onAddedToRoot(UIRoot& root)
{
	root.setFocus(getWidget("name"));
}

void NewAssetWindow::makeUI()
{
	add(factory.makeUI("halley/new_asset_window"));

	getWidgetAs<UILabel>("title")->setText(label);

	const auto name = getWidgetAs<UITextInput>("name");
	name->setValidator(std::make_shared<FileNameValidator>());
	name->setText(startValue);
	name->setGhostText(LocalisedString::fromUserString(startValue.isEmpty() ? "" : (startValue + extension)));
	name->setAppendText(LocalisedString::fromUserString(extension));

	getWidget("ok")->setEnabled(!startValue.isEmpty());

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		callback(name->getText().isEmpty() ? std::optional<String>() : String(name->getText()));
		destroy();
	});
	
	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event)
	{
		callback({});
		destroy();
	});

	setHandle(UIEventType::TextChanged, "name", [=] (const UIEvent& event)
	{
		getWidget("ok")->setEnabled(!event.getStringData().isEmpty());
	});

	setHandle(UIEventType::TextSubmit, "name", [=] (const UIEvent& event)
	{
		callback(event.getStringData().isEmpty() ? std::optional<String>() : event.getStringData());
		destroy();
	});
}

StringUTF32 FileNameValidator::onTextChanged(StringUTF32 changedTo)
{
	const StringUTF32 illegalChars = U"./\\?:\"<>|*";

	StringUTF32 result;
	result.reserve(changedTo.size());
	for (auto c: changedTo) {
		if (c >= 32 && illegalChars.find(c) == StringUTF32::npos) {
			result.push_back(c);
		}
	}
	return result;
}
