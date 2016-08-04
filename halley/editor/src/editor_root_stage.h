#pragma once

#include "prec.h"

class EditorRootStage final : public Halley::Stage
{
public:
	void init() override;
	void onVariableUpdate(Halley::Time time) override;
	void onRender(Halley::RenderContext& context) const override;

private:
	Halley::Sprite halleyLogo;
};
