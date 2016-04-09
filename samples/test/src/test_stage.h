#pragma once

#include "prec.h"

class TestStage : public Halley::Stage
{
public:
	void init() override;
	void deInit() override;
	void onVariableUpdate(Halley::Time time) override;
	void onFixedUpdate(Halley::Time time) override;
	void onRender(Halley::Painter& painter) const override;

private:
	Halley::World world;
	int i = 0;
	EntityId id0;
	EntityId id2;
};
