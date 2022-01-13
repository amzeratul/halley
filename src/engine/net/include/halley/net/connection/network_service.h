#pragma once
#include <memory>
#include <halley/text/halleystring.h>
#include "iconnection.h"

namespace Halley
{
	enum class IPVersion
	{
		IPv4,
		IPv6
	};

	class NetworkService
	{
		public:
		class Acceptor {
		public:
			virtual ~Acceptor() = default;

			std::shared_ptr<IConnection> accept();
			void reject();
			void ensureChoiceMade();

		private:
			bool choiceMade = false;

			virtual std::shared_ptr<IConnection> doAccept() = 0;
			virtual void doReject() = 0;
		};

		using AcceptCallback = std::function<void(Acceptor&)>;
		
		virtual ~NetworkService() = default;

		virtual void update() = 0;

		virtual void startListening(AcceptCallback callback) = 0;
        virtual void stopListening() = 0;
		virtual std::shared_ptr<IConnection> connect(const String& address) = 0;
	};
}
