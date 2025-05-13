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
{
	UIWidget::setAnchor(UIAnchor());
	factory.loadUI(*this, "halley/localisation/localisation_users");
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
	client.getUsers().then(aliveFlag, Executors::getMainUpdateThread(), [=] (Vector<LocUserData> users) {
		populateUserList(std::move(users));
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
	const auto& userInfo = getUser(currentUserId);
	curUserDataWorkingCopy = userInfo.getProject(client.getProject()); // Copy

	getWidgetAs<UILabel>("username")->setText(LocalisedString::fromUserString(userInfo.username));
	getWidgetAs<UIButton>("toggleAdmin")->setLabel(LocalisedString::fromUserString(userInfo.isAdmin ? "Remove Admin" : "Make Admin"));
	getWidgetAs<UIButton>("deleteUser")->setEnabled(!userInfo.isAdmin);
	getWidgetAs<UIButton>("updateLanguages")->setEnabled(false);

	auto languageContainer = getWidget("languages");
	languageContainer->clear();
	for (const auto& lang: project.getProperties().getLanguages()) {
		const auto isoCode = lang.getISOCode();

		const auto enabled = curUserDataWorkingCopy.languages.contains(isoCode);
		auto checkbox = std::make_shared<UICheckbox>(isoCode, factory.getStyle("checkbox"), enabled);
		checkbox->setHandle(UIEventType::CheckboxUpdated, [=] (const UIEvent& event) {
			setLanguageEnabled(lang, event.getBoolData());
		});
		auto icon = std::make_shared<UIImage>(editorRoot.getFlag(lang));
		auto label = std::make_shared<UILabel>("", factory.getStyle("label"), editorRoot.getLanguageName(lang));

		auto entry = std::make_shared<UIWidget>("", Vector2f(), UISizer());
		entry->add(checkbox, 0, Vector4f(0, 0, 4, 0), UISizerAlignFlags::Centre);
		entry->add(icon, 0, Vector4f(0, 0, 4, 0), UISizerAlignFlags::Centre);
		entry->add(label, 0, {}, UISizerAlignFlags::Centre);
		languageContainer->add(std::move(entry));
	}
}

void LocalisationManageUsers::setLanguageEnabled(const I18NLanguage& language, bool enabled)
{
	const auto code = language.getISOCode();
	if (enabled) {
		if (!curUserDataWorkingCopy.languages.contains(code)) {
			curUserDataWorkingCopy.languages.push_back(code);
		}
	} else {
		std_ex::erase(curUserDataWorkingCopy.languages, code);
	}

	getWidgetAs<UIButton>("updateLanguages")->setEnabled(curUserDataWorkingCopy != getUser(currentUserId).getProject(client.getProject()));
}

const LocUserData& LocalisationManageUsers::getUser(const String& userId)
{
	const auto userIter = curUsers.find_if([&] (const auto& u) { return u.username == userId; });
	if (userIter == curUsers.end()) {
		static LocUserData dummy;
		return dummy;
	}
	return *userIter;
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
	getWidgetAs<UIButton>("updateLanguages")->setEnabled(false);
	client.putUserProjectSettings(currentUserId, curUserDataWorkingCopy.languages).then(aliveFlag, Executors::getMainUpdateThread(), [=] (bool ok) {
		if (!ok) {
			setCurrentUser(currentUserId);
		}
	});
}

