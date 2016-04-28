#pragma once

#include "prec.h"

class TestStage : public Halley::Stage
{
public:
	void init() override;
	void deInit() override;
	void onFixedUpdate(Halley::Time time) override;
	void onRender(Halley::RenderContext& context) const override;

private:
	std::unique_ptr<Halley::World> world;
	std::shared_ptr<Halley::TextureRenderTarget> target;

	void spawnTestSprite();
};
