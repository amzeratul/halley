#pragma once

#include <memory>

namespace Halley
{
	class Resources;
	class UIFactory;
	class UIWidget;
	class String;
	class ResourceObserver;
	class UIParent;

	class UIFactoryTester
	{
	public:
		UIFactoryTester(UIFactory& factory, UIParent& parent, Resources& resources);
		~UIFactoryTester();

		void update();

		void loadUI(const String& id);

	private:
		UIFactory& factory;
		UIParent& parent;
		Resources& resources;

		std::shared_ptr<UIWidget> curUI;
		std::unique_ptr<ResourceObserver> curObserver;

		void loadFromObserver();
	};
}
