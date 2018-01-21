#pragma once

#include "prec.h"

class TestStage final : public Halley::EntityStage
{
public:
	void init() override;
	void onVariableUpdate(Halley::Time time) override;
	void onRender(Halley::RenderContext& context) const override;

private:
	bool playingMusic = true;
	float pan = 0.0f;
};
