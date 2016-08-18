#pragma once
#include <vector>
#include <gsl/gsl>

namespace Halley
{
	class NetworkPacket
	{
	public:
		NetworkPacket();
		explicit NetworkPacket(gsl::span<const gsl::byte> src);

		NetworkPacket(const NetworkPacket& packet) = default;
		NetworkPacket(NetworkPacket&& packet) = default;
		NetworkPacket& operator=(const NetworkPacket& packet) = default;
		NetworkPacket& operator=(NetworkPacket&& packet) = default;

		size_t copyTo(gsl::span<gsl::byte> dst) const;

		size_t getSize() const;
		const char* getData() const;

		void addHeader(const gsl::span<gsl::byte> src);
		void extractHeader(gsl::span<gsl::byte> dst);

	private:
		std::vector<char> data;
	};
}
