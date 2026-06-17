#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

TEST(HalleyRingBuffer, WriteOneReadOneST)
{
	auto buffer = RingBuffer<int>(128);

	int nRounds = 200000;

	for (int i = 0; i < nRounds; ++i) {
		EXPECT_TRUE(buffer.canWrite(1));
		buffer.writeOne(i);

		EXPECT_EQ(buffer.availableToRead(), 1);
		const auto read = buffer.readOne();
		EXPECT_EQ(read, i);
		EXPECT_EQ(buffer.availableToRead(), 0);
	}
}

TEST(HalleyRingBuffer, WriteOneReadOneBatchST)
{
	auto buffer = RingBuffer<int>(256);

	auto rng = Random(Random::getSharedGlobal().getRawInt());
	int nRounds = 20000;

	int nWritten = 0;
	int nRead = 0;

	for (int i = 0; i < nRounds; ++i) {
		const int toWrite = std::min(rng.getInt(10, 100), static_cast<int>(buffer.availableToWrite()));
		for (int j = 0; j < toWrite; ++j) {
			buffer.writeOne(nWritten++);
		}

		const int toRead = std::min(rng.getInt(10, 100), static_cast<int>(buffer.availableToRead()));
		for (int j = 0; j < toRead; ++j) {
			auto v = buffer.readOne();
			EXPECT_EQ(v, nRead++);
		}
	}

	const int toRead = static_cast<int>(buffer.availableToRead());
	for (int j = 0; j < toRead; ++j) {
		auto v = buffer.readOne();
		EXPECT_EQ(v, nRead++);
	}
	EXPECT_EQ(buffer.availableToRead(), 0);
}

TEST(HalleyRingBuffer, WriteReadBatchST)
{
	constexpr int bufferSize = 256;
	auto buffer = RingBuffer<int>(bufferSize);

	auto rng = Random(Random::getSharedGlobal().getRawInt());
	int nRounds = 200000;

	int nWritten = 0;
	int nRead = 0;

	std::array<int, bufferSize> writeBufMem;
	std::array<int, bufferSize> readBufMem;
	auto writeBuf = gsl::span(writeBufMem);
	auto readBuf = gsl::span(readBufMem);

	for (int i = 0; i < nRounds; ++i) {
		const int toWrite = std::min(rng.getInt(10, 100), static_cast<int>(buffer.availableToWrite()));
		for (int j = 0; j < toWrite; ++j) {
			writeBuf[j] = nWritten++;
		}
		buffer.write(writeBuf.subspan(0, toWrite));

		const int toRead = std::min(rng.getInt(10, 100), static_cast<int>(buffer.availableToRead()));
		buffer.read(readBuf.subspan(0, toRead));
		for (int j = 0; j < toRead; ++j) {
			EXPECT_EQ(readBuf[j], nRead++);
		}
	}

	const int toRead = static_cast<int>(buffer.availableToRead());
	buffer.read(writeBuf.subspan(0, toRead));
	for (int j = 0; j < toRead; ++j) {
		EXPECT_EQ(writeBuf[j], nRead++);
	}
	EXPECT_EQ(buffer.availableToRead(), 0);
}

TEST(HalleyRingBuffer, WriteOneReadOneMT)
{
	auto buffer = RingBuffer<int>(4096);

	const int nRounds = 100000000;

	// Write thread
	auto writeThread = std::thread([&] ()
	{
		for (int i = 0; i < nRounds; ) {
			if (buffer.canWrite(1)) {
				buffer.writeOne(i++);
			} else {
				std::this_thread::yield();
			}
		}
	});

	// Read thread
	auto readThread = std::thread([&] ()
	{
		for (int i = 0; i < nRounds; ) {
			if (buffer.canRead(1)) {
				auto v = buffer.readOne();
				EXPECT_EQ(v, i++);
			} else {
				std::this_thread::yield();
			}
		}
	});

	writeThread.join();
	readThread.join();

	EXPECT_EQ(buffer.availableToRead(), 0);
}

TEST(HalleyRingBuffer, WriteReadBatchMT)
{
	auto buffer = RingBuffer<int>(4096);

	const int nRounds = 100000000;

	// Write thread
	auto writeThread = std::thread([&] ()
	{
		auto rng = Random(Random::getGlobal().getRawInt());
		int writeBufMem[128];
		auto writeBuf = gsl::span(std::begin(writeBufMem), std::end(writeBufMem));

		for (int i = 0; i < nRounds; ) {
			const int toWrite = std::min(rng.getInt(10, 100), nRounds - i);
			for (int j = 0; j < toWrite; ++j) {
				writeBuf[j] = i++;
			}

			while (!buffer.canWrite(toWrite)) {
				std::this_thread::yield();
			}
			buffer.write(writeBuf.subspan(0, toWrite));
		}
	});

	// Read thread
	auto readThread = std::thread([&] ()
	{
		auto rng = Random(Random::getGlobal().getRawInt());
		int readBufMem[128];
		auto readBuf = gsl::span(std::begin(readBufMem), std::end(readBufMem));

		for (int i = 0; i < nRounds; ) {
			const int toRead = std::min(rng.getInt(10, 100), nRounds - i);
			while (!buffer.canRead(toRead)) {
				std::this_thread::yield();
			}

			buffer.read(readBuf.subspan(0, toRead));
			for (int j = 0; j < toRead; ++j) {
				EXPECT_EQ(readBuf[j], i++);
			}
		}
	});

	writeThread.join();
	readThread.join();

	EXPECT_EQ(buffer.availableToRead(), 0);
}
