#pragma once

#include <halley.hpp>

namespace Halley {
	class LauncherStage : public Stage {
	public:
		void init() override;

		void onVariableUpdate(Time) override;
		void onRender(RenderContext&) const override;

	private:
		I18N i18n;
		std::unique_ptr<UIFactory> uiFactory;
		std::unique_ptr<UIRoot> ui;
		std::shared_ptr<UIWidget> topLevelUI;

		void makeUI();
		void updateUI(Time time);
	};
}