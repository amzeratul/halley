#pragma once

#include "prec.h"

class TestStage final : public Halley::Stage
{
public:
	TestStage();
	void init() override;
	void onFixedUpdate(Halley::Time time) override;
	void onRender(Halley::RenderContext& context) const override;

private:
	
	void updateNetwork();

	std::unique_ptr<Halley::NetworkService> network;
	Halley::IConnection* connection;
};
