#pragma once

#include "prec.h"

namespace Halley {
	class ConsoleWindow;

	class EditorRootStage final : public Stage
	{
	public:
		EditorRootStage();
		~EditorRootStage();
		void init() override;
		void onVariableUpdate(Time time) override;
		void onRender(RenderContext& context) const override;

	private:
		Sprite halleyLogo;
		std::unique_ptr<ConsoleWindow> console;
	};
}
