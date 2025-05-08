#include "localisation_manage_users.h"

using namespace Halley;

LocalisationManageUsers::LocalisationManageUsers(UIFactory& factory, LocalisationClient& client)
	: UIWidget("manage_users", {}, UISizer())
	, factory(factory)
	, client(client)
	, aliveFlag(std::make_shared<bool>(true))
{
	UIWidget::setAnchor(UIAnchor());
	factory.loadUI(*this, "halley/localisation/localisation_users");
}

LocalisationManageUsers::~LocalisationManageUsers()
{
	*aliveFlag = false;
}

void LocalisationManageUsers::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "close", [=] (const UIEvent& event) {
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "addUser", [=] (const UIEvent& event) {
		addUser();
	});

	setHandle(UIEventType::ButtonClicked, "deleteUser", [=] (const UIEvent& event) {
		deleteUser();
	});

	setHandle(UIEventType::ButtonClicked, "changePassword", [=] (const UIEvent& event) {
		changePassword();
	});

	setHandle(UIEventType::ButtonClicked, "makeAdmin", [=] (const UIEvent& event) {
		toggleAdmin();
	});

	setHandle(UIEventType::ButtonClicked, "updateLanguages", [=] (const UIEvent& event) {
		updateLanguages();
	});

	setHandle(UIEventType::ListSelectionChanged, "users", [=] (const UIEvent& event) {
		setCurrentUser(event.getStringData());
	});

	populateList();
}

void LocalisationManageUsers::populateList()
{
	// TODO
}

void LocalisationManageUsers::setCurrentUser(String user)
{
	currentUser = std::move(user);

	getWidgetAs<UILabel>("username")->setText(LocalisedString::fromUserString(user));
}

void LocalisationManageUsers::addUser()
{
	// TODO
}

void LocalisationManageUsers::deleteUser()
{
	// TODO
}

void LocalisationManageUsers::toggleAdmin()
{
	// TODO
}

void LocalisationManageUsers::changePassword()
{
	// TODO
}

void LocalisationManageUsers::updateLanguages()
{
	// TODO
}

