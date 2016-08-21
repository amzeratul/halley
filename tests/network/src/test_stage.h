#pragma once

#include "prec.h"

class TestStage final : public Halley::Stage
{
public:
	TestStage();
	void init() override;
	void onVariableUpdate(Halley::Time time) override;
	void onRender(Halley::RenderContext& context) const override;

private:
	
	void updateNetwork();
	void setConnection(std::shared_ptr<Halley::IConnection> connection);

	std::unique_ptr<Halley::NetworkService> network;
	std::shared_ptr<Halley::ReliableConnection> connection;
	std::unique_ptr<Halley::MessageQueue> msgs;
	bool isClient;

	int count = 0;
};
