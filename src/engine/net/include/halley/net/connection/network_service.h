#pragma once
#include <memory>
#include <halley/text/halleystring.h>
#include "iconnection.h"
#include "halley/time/halleytime.h"
#include "halley/text/string_converter.h"

namespace Halley
{
	enum class IPVersion
	{
		IPv4,
		IPv6
	};

	class NetworkService : public IConnectionStatsListener
	{
	public:
		enum class Quality {
			Best,
			Average,
			Bad,
			VeryBad
		};
		
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

		virtual void update(Time t) {}

		virtual String startListening(AcceptCallback callback) = 0; // Returns the address that clients will use to connect to
        virtual void stopListening() = 0;
		virtual std::shared_ptr<IConnection> connect(const String& address) = 0;

		virtual void setSimulateQualityLevel(Quality quality) {}
	};

	template <>
	struct EnumNames<NetworkService::Quality> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"best",
				"average",
				"bad",
				"veryBad"
			}};
		}
	};

	class NetworkServiceWithStats : public NetworkService {
	public:
		virtual ~NetworkServiceWithStats() = default;

		void update(Time t) override;

		void onSendData(size_t size, size_t nPackets) override;
		void onReceiveData(size_t size, size_t nPackets) override;
		size_t getSentDataPerSecond() const override;
		size_t getReceivedDataPerSecond() const override;
		size_t getSentPacketsPerSecond() const override;
		size_t getReceivedPacketsPerSecond() const override;

	protected:
		size_t sentSize = 0;
		size_t receivedSize = 0;
		size_t sentPackets = 0;
		size_t receivedPackets = 0;

		virtual void onUpdateStats();
		virtual Time getStatUpdateInterval() const;

	private:
		Time statsTime = 0.0;
		size_t lastSentSize = 0;
		size_t lastReceivedSize = 0;
		size_t lastSentPackets = 0;
		size_t lastReceivedPackets = 0;
	};
}
