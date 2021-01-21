#pragma once
#include "task_display.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class TaskDetails : public UIWidget {
	public:
		TaskDetails(UIFactory& factory, std::shared_ptr<IClipboard> clipboard);

		void onMakeUI() override;

		void show(const TaskDisplay& taskDisplay);
		void hide();

		void onMouseOver(Vector2f mousePos) override;
		bool canInteractWithMouse() const override;
	
	protected:
		void update(Time t, bool moved) override;
		void drawChildren(UIPainter& painter) const override;
	
	private:
		UIFactory& factory;
		std::shared_ptr<IClipboard> clipboard;

		bool visible = false;
		const TaskDisplay* taskDisplay = nullptr;
		Time showTime = 0;
		size_t lastNumMessages = 0;
		TaskStatus lastStatus = TaskStatus::WaitingToStart;

		void updateMessages();
		void copyToClipboard(bool verbose);
	};
}
