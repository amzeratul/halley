#pragma once
#include <memory>
#include <vector>
#include "halley/text/halleystring.h"
#include "halley/maths/rect.h"

namespace Halley {
	enum class UIInputType;
	class UIEvent;
	class UIWidget;
	class UIRoot;

	class UIParent {
	public:
		virtual ~UIParent();

		virtual const String& getId() const = 0;
		virtual UIRoot* getRoot() = 0;
		virtual const UIRoot* getRoot() const = 0;
		virtual void sendEvent(UIEvent&& event) const = 0;

		virtual Rect4f getRect() const = 0;

		void addChild(std::shared_ptr<UIWidget> widget);
		void removeChild(UIWidget& widget);

		bool addNewChildren(UIInputType inputType);
		bool removeDeadChildren();
		bool isWaitingToSpawnChildren() const;

		virtual void markAsNeedingLayout();

		std::vector<std::shared_ptr<UIWidget>>& getChildren();
		const std::vector<std::shared_ptr<UIWidget>>& getChildren() const;

		std::shared_ptr<UIWidget> getWidget(const String& id);
		std::shared_ptr<UIWidget> tryGetWidget(const String& id);
		
		template <typename T>
		std::shared_ptr<T> tryGetWidgetAs(const String& id)
		{
			return std::dynamic_pointer_cast<T>(tryGetWidget(id));
		}

		template <typename T>
		std::shared_ptr<T> getWidgetAs(const String& id)
		{
			auto widget = getWidget(id);
			if (widget) {
				auto w = std::dynamic_pointer_cast<T>(widget);
				if (!w) {
					throw Exception("Widget with id \"" + id + "\" was found, but it is not of type " + String(typeid(T).name()), HalleyExceptions::UI);
				}
				return w;
			} else {
				return {};
			}
		}

		virtual bool isDescendentOf(const UIWidget& ancestor) const;
		
	private:
		std::vector<std::shared_ptr<UIWidget>> children;
		std::vector<std::shared_ptr<UIWidget>> childrenWaiting;

		std::shared_ptr<UIWidget> doGetWidget(const String& id) const;
	};
}
