#pragma once

#include "prec.h"

class TestStage final : public Halley::EntityStage
{
public:
	void init() override;
	void onFixedUpdate(Halley::Time time) override;
	void onVariableUpdate(Halley::Time time) override;
	void onRender(Halley::RenderContext& context) const override;

private:
	std::unique_ptr<Halley::World> world;
	std::unique_ptr<Halley::WorldStatsView> statsView;
	//std::shared_ptr<Halley::TextureRenderTarget> target;
};
