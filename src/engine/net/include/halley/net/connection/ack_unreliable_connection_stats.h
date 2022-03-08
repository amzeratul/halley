#pragma once
#include "ack_unreliable_connection.h"
#include "halley/time/halleytime.h"

namespace Halley {
    class AckUnreliableConnectionStats : public IAckUnreliableConnectionStatsListener {
    public:
        enum class State {
        	Sent,
        	Resent,
        	Acked
        };

        struct PacketStats {
	        uint16_t seq = 0;
            State state = State::Sent;
        };

    	void update(Time time);

    	void onPacketSent(uint16_t sequence) override;
	    void onPacketResent(uint16_t sequence) override;
	    void onPacketAcked(uint16_t sequence) override;

        [[nodiscard]] gsl::span<const PacketStats> getPacketStats() const;

    private:
        std::vector<PacketStats> packetStats;
    };
}
