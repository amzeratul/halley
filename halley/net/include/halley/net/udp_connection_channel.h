#pragma once
#include <halley/utils/utils.h>
#include <map>

namespace Halley
{
	class UDPConnection;

	class UDPConnectionChannel
	{
	public:
		UDPConnectionChannel(UDPConnection& parent, bool reliable, bool ordered);

		int nextMessageNumber();

		void onGotAcks(std::vector<int>&& msgs);
		std::vector<int> getAcksExpected() const;
		
		void onGotAckRequest(std::vector<int>&& msgs);
		std::vector<int> getAcksToSend();

		void send(Bytes&& msg, int number);
		void receive(Bytes&& msg, int number);

	private:
		UDPConnection& parent;
		bool reliable;
		bool ordered;

		int lastMessageSent = 0;
		std::map<int, Bytes> sentPendingAck;

		int receivedAllUpTo = 0;
		std::map<int, Bytes> receivedPending;

		std::vector<int> toAck;
	};
}
