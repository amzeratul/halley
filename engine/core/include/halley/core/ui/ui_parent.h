#pragma once
#include <memory>
#include <vector>

namespace Halley {
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
		void removeDeadChildren();

		std::vector<std::shared_ptr<UIWidget>>& getChildren();
		const std::vector<std::shared_ptr<UIWidget>>& getChildren() const;
		
	protected:
		bool topChildChanged = false;

	private:
		std::vector<std::shared_ptr<UIWidget>> children;
	};
}
