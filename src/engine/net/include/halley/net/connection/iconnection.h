#pragma once

namespace Halley
{
	class InboundNetworkPacket;
	class OutboundNetworkPacket;

	enum class ConnectionStatus
	{
		Undefined,
		Connecting,
		Connected,
		Closing,
		Closed
	};

	class IConnectionStatsListener {
	public:
		virtual ~IConnectionStatsListener() = default;
		virtual void onSendData(size_t size, size_t nPackets) = 0;
		virtual void onReceiveData(size_t size, size_t nPackets) = 0;

		virtual size_t getSentDataPerSecond() const = 0;
		virtual size_t getReceivedDataPerSecond() const = 0;
		virtual size_t getSentPacketsPerSecond() const = 0;
		virtual size_t getReceivedPacketsPerSecond() const = 0;
	};

	class IConnection
	{
	public:
		enum class TransmissionType {
			Unreliable,
			Reliable
		};
		
		virtual ~IConnection() {}

		virtual void close() = 0;
		virtual ConnectionStatus getStatus() const = 0;

		virtual bool isSupported(TransmissionType type) const = 0;
		
		virtual void send(TransmissionType type, OutboundNetworkPacket packet) = 0;
		virtual bool receive(InboundNetworkPacket& packet) = 0;
	};
}
