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
		std::shared_ptr<UIWidget> curUI;

		Sprite background;

		void makeSprites();
		void makeUI();
		void updateUI(Time time);
		void setCurrentUI(std::shared_ptr<UIWidget> ui);
	};
}