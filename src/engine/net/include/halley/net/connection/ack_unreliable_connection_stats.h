#pragma once
#include "ack_unreliable_connection.h"
#include "halley/time/halleytime.h"

namespace Halley {
    class AckUnreliableConnectionStats : public IAckUnreliableConnectionStatsListener {
    public:
        enum class State : uint8_t {
            Unsent,
        	Sent,
        	Resent,
        	Acked
        };

        struct PacketStats {
	        uint16_t seq = 0;
            State state = State::Unsent;
            size_t size;
        };

        AckUnreliableConnectionStats(size_t capacity, size_t lineSize);

    	void update(Time time);

    	void onPacketSent(uint16_t sequence, size_t size) override;
	    void onPacketResent(uint16_t sequence) override;
	    void onPacketAcked(uint16_t sequence) override;

        [[nodiscard]] gsl::span<const PacketStats> getPacketStats() const;
        [[nodiscard]] size_t getLineStart() const;
        [[nodiscard]] size_t getLineSize() const;

    private:
        size_t capacity;
        size_t lineSize;
        size_t lineStart;

        std::vector<PacketStats> packetStats;
        size_t pos = 0; 
    };
}
