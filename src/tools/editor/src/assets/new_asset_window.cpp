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
	root.registerKeyPressListener(shared_from_this(), 1);
}

void NewAssetWindow::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

bool NewAssetWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Enter)) {
		accept();
	}

	if (key.is(KeyCode::Esc)) {
		cancel();
	}

	return true;
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
		accept();
	});
	
	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event)
	{
		cancel();
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

void NewAssetWindow::accept()
{
	const auto name = getWidgetAs<UITextInput>("name");
	callback(name->getText().isEmpty() ? std::optional<String>() : String(name->getText()));
	destroy();
}

void NewAssetWindow::cancel()
{
	callback({});
	destroy();
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
