#pragma once
#include <memory>
#include <vector>
#include "halley/text/halleystring.h"

namespace Halley {
	enum class UIInputType;
	class UIEvent;
	class UIWidget;
	class UIRoot;

	class UIParent {
	public:
		virtual ~UIParent() {}

		virtual UIRoot* getRoot() = 0;
		virtual void sendEvent(UIEvent&& event) const = 0;

		void addChild(std::shared_ptr<UIWidget> widget);
		void removeChild(UIWidget& widget);

		bool addNewChildren(UIInputType inputType);
		bool removeDeadChildren();

		std::vector<std::shared_ptr<UIWidget>>& getChildren();
		const std::vector<std::shared_ptr<UIWidget>>& getChildren() const;

		std::shared_ptr<UIWidget> getWidget(const String& id);
		
		template <typename T>
		std::shared_ptr<T> getWidgetAs(const String& id)
		{
			return std::static_pointer_cast<T>(getWidget(id));
		}
		
	protected:
		bool topChildChanged = false;

	private:
		std::vector<std::shared_ptr<UIWidget>> children;
		std::vector<std::shared_ptr<UIWidget>> childrenWaiting;
	};
}
