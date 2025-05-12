#include "localisation_manage_users.h"

#include "localisation_editor_root.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"

using namespace Halley;

LocalisationManageUsers::LocalisationManageUsers(UIFactory& factory, LocalisationClient& client, Project& project, LocalisationEditorRoot& editorRoot)
	: UIWidget("manage_users", {}, UISizer())
	, factory(factory)
	, client(client)
	, project(project)
	, editorRoot(editorRoot)
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

	setHandle(UIEventType::ButtonClicked, "toggleAdmin", [=] (const UIEvent& event) {
		toggleAdmin();
	});

	setHandle(UIEventType::ButtonClicked, "updateLanguages", [=] (const UIEvent& event) {
		updateLanguages();
	});

	setHandle(UIEventType::ListSelectionChanged, "users", [=] (const UIEvent& event) {
		setCurrentUser(event.getStringData());
	});

	requestUserList();
}

void LocalisationManageUsers::requestUserList()
{
	client.getUsers().then(Executors::getMainUpdateThread(), [=] (Vector<LocUserData> users) {
		if (*aliveFlag) {
			populateUserList(std::move(users));
		}
	});
}

void LocalisationManageUsers::populateUserList(Vector<LocUserData> users)
{
	if (users == curUsers) {
		return;
	}
	curUsers = std::move(users);

	auto userList = getWidgetAs<UIList>("users");
	userList->clear();

	const auto curUserId = userList->getSelectedOptionId();
	userList->setCanSendEvents(false);

	for (const auto& user: curUsers) {
		userList->addTextItem(user.username, LocalisedString::fromUserString(user.username + (user.isAdmin ? " [admin]" : "")));
	}

	userList->setSelectedOptionId(curUserId);
	userList->setCanSendEvents(true);

	setCurrentUser(userList->getSelectedOptionId());
}

void LocalisationManageUsers::setCurrentUser(String _userId)
{
	currentUserId = std::move(_userId);
	const auto userIter = curUsers.find_if([&] (const auto& u) { return u.username == currentUserId; });
	const auto& userInfo = userIter == curUsers.end() ? LocUserData() : *userIter;

	getWidgetAs<UILabel>("username")->setText(LocalisedString::fromUserString(userInfo.username));
	getWidgetAs<UIButton>("toggleAdmin")->setLabel(LocalisedString::fromUserString(userInfo.isAdmin ? "Remove Admin" : "Make Admin"));
	getWidgetAs<UIButton>("deleteUser")->setEnabled(!userInfo.isAdmin);
	getWidgetAs<UIButton>("updateLanguages")->setEnabled(false); // TODO

	Vector<String> languagesEnabled;
	const auto projIter = userInfo.projects.find(client.getProject());
	if (projIter != userInfo.projects.end()) {
		languagesEnabled = projIter->second.languages;
	}

	auto languageContainer = getWidget("languages");
	languageContainer->clear();
	for (const auto& lang: project.getProperties().getLanguages()) {
		const auto isoCode = lang.getISOCode();

		const auto enabled = languagesEnabled.contains(isoCode);
		auto checkbox = std::make_shared<UICheckbox>(isoCode, factory.getStyle("checkbox"), enabled);
		auto icon = std::make_shared<UIImage>(editorRoot.getFlag(lang));
		auto label = std::make_shared<UILabel>("", factory.getStyle("label"), editorRoot.getLanguageName(lang));

		auto entry = std::make_shared<UIWidget>("", Vector2f(), UISizer());
		entry->add(checkbox, 0, Vector4f(0, 0, 4, 0), UISizerAlignFlags::Centre);
		entry->add(icon, 0, Vector4f(0, 0, 4, 0), UISizerAlignFlags::Centre);
		entry->add(label, 0, {}, UISizerAlignFlags::Centre);
		languageContainer->add(std::move(entry));
	}
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

