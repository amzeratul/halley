#pragma once

#include "iconnection.h"
#include <memory>

namespace Halley
{
	class ReliableConnection : public IConnection
	{
	public:
		ReliableConnection(std::shared_ptr<IConnection> parent);

		void close() override;
		ConnectionStatus getStatus() const override;
		void send(NetworkPacket&& packet) override;
		bool receive(NetworkPacket& packet) override;

	private:
		std::shared_ptr<IConnection> parent;
	};
}