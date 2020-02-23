#pragma once

#include <halley.hpp>
using namespace Halley;

class GameStage : public EntityStage {
public:
	void init() override;

	void onVariableUpdate(Time) override;
	void onRender(RenderContext&) const override;
};
