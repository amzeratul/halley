#pragma once

#include <halley.hpp>

#include "launcher_save_data.h"

namespace Halley {
	class LauncherStage : public Stage {
	public:
		LauncherStage();
		
		void init() override;

		void onVariableUpdate(Time) override;
		void onRender(RenderContext&) const override;

	private:
		I18N i18n;
		std::unique_ptr<UIFactory> uiFactory;
		std::unique_ptr<UIRoot> ui;
		std::shared_ptr<UIWidget> topLevelUI;
		std::shared_ptr<UIWidget> curUI;

		Executor mainThreadExecutor;

		Sprite background;
		std::shared_ptr<LauncherSaveData> saveData;

		void makeSprites();
		void makeUI();
		void updateUI(Time time);
		void setCurrentUI(std::shared_ptr<UIWidget> ui);
	};
}
