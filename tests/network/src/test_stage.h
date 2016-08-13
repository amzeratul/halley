#pragma once

#include "prec.h"

class TestStage final : public Halley::Stage
{
public:
	void init() override;
	void onFixedUpdate(Halley::Time time) override;
	void onRender(Halley::RenderContext& context) const override;

private:
	
	void updateNetwork();

	Halley::NetworkService network;
	bool networkInit = false;
	std::shared_ptr<Halley::UDPConnection> connection;
};
