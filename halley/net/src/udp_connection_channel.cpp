#include "udp_connection_channel.h"

Halley::UDPConnectionChannel::UDPConnectionChannel(UDPConnection& parent, bool reliable, bool ordered)
	: parent(parent)
	, reliable(reliable)
	, ordered(ordered)
{
}

int Halley::UDPConnectionChannel::nextMessageNumber()
{
	return ++lastMessageSent;
}

void Halley::UDPConnectionChannel::onGotAcks(std::vector<int>&& msgs)
{
	for (int msg: msgs) {
		sentPendingAck.erase(msg);
	}
}

std::vector<int> Halley::UDPConnectionChannel::getAcksExpected() const
{
	std::vector<int> result;
	for (auto& msg: sentPendingAck) {
		result.push_back(msg.first);
	}
	return result;
}

void Halley::UDPConnectionChannel::onGotAckRequest(std::vector<int>&& msgs)
{
	std::sort(msgs.begin(), msgs.end());
	std::sort(toAck.begin(), toAck.end());
	std::vector<int> result(msgs.size() + toAck.size());
	auto pos = std::set_union(msgs.begin(), msgs.end(), toAck.begin(), toAck.end(), result.begin());
	result.resize(pos - result.begin());
	toAck = std::move(result);
}

std::vector<int> Halley::UDPConnectionChannel::getAcksToSend()
{
	auto result = std::move(toAck);
	std::sort(result.begin(), result.end());
	return result;
}

void Halley::UDPConnectionChannel::send(Bytes&& msg, int number)
{
	sentPendingAck[number] = std::move(msg);

	// TODO: actually send
}

void Halley::UDPConnectionChannel::receive(Bytes&& msg, int number)
{
	// TODO: check if this was already received

	toAck.push_back(number);
	receivedPending[number] = std::move(msg);
}
