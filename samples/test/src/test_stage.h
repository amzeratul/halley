#pragma once

#include "prec.h"

class TestStage : public Halley::Stage
{
public:
	void init() override;
	void deInit() override;
	void onVariableUpdate(Halley::Time time) override;
	void onFixedUpdate(Halley::Time time) override;
	void onRender(Halley::RenderContext& context) const override;

private:
	Halley::World world;
	int i = 0;
	EntityId id0;
	EntityId id2;
	float curTime = 0;

	Halley::Sprite sprite;

	std::shared_ptr<Halley::TextureRenderTarget> target;
};
