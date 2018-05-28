#include "ui_factory_tester.h"
#include "ui_factory.h"
#include "ui_anchor.h"
#include "ui_widget.h"
#include "ui_parent.h"
#include "halley/file_formats/config_file.h"
#include "halley/support/logger.h"
#include "halley/core/resources/resources.h"

using namespace Halley;

UIFactoryTester::UIFactoryTester(UIFactory& factory, UIParent& parent, Resources& resources)
	: factory(factory)
	, parent(parent)
	, resources(resources)
{}

UIFactoryTester::~UIFactoryTester() = default;

void UIFactoryTester::update()
{
	if (curObserver && curObserver->needsUpdate()) {
		curObserver->update();
		loadFromObserver();
	}
}

void UIFactoryTester::loadUI(const String& uiName)
{
	curObserver = uiName.isEmpty() ? std::unique_ptr<ConfigObserver>() : std::make_unique<ConfigObserver>(*resources.get<ConfigFile>(uiName));
	loadFromObserver();
}

void UIFactoryTester::loadFromObserver()
{
	if (curUI) {
		curUI->destroy();
		curUI.reset();
	}

	if (curObserver) {
		try {
			curUI = factory.makeUIFromNode(curObserver->getRoot());
			curUI->setAnchor(UIAnchor());
			curUI->setMouseBlocker(false);
			parent.addChild(curUI);
		} catch (std::exception& e) {
			Logger::logException(e);
		} catch (...) {
			Logger::logError("Unknown exception loading UI");
		}
	}
}
