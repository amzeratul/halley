#include "audio_source_clip.h"
#include <utility>
#include "halley/audio/audio_clip.h"
#include "../audio_mixer.h"
#include "../audio_engine.h"
#include "halley/concurrency/executor.h"

using namespace Halley;


AudioSourceClip::AudioSourceClip(AudioEngine& engine, std::shared_ptr<const IAudioClip> c, bool looping, float gain, int64_t loopStart, int64_t loopEnd, bool randomiseStart)
	: engine(engine)
	, clip(std::move(c))
	, loopStart(loopStart)
	, loopEnd(loopEnd)
	, gain(gain)
	, prevGain(gain)
	, looping(looping)
	, randomiseStart(randomiseStart)
{
	HalleyAssertDev(clip != nullptr);
}

String AudioSourceClip::getName() const
{
	return clip->getName();
}

uint8_t AudioSourceClip::getNumberOfChannels() const
{
	return clip->getNumberOfChannels();
}

bool AudioSourceClip::isReady() const
{
	return clip->isLoaded();
}

size_t AudioSourceClip::getSamplesLeft() const
{
	return looping ? std::numeric_limits<size_t>::max() : (clip->getLength() - streams[0].playbackPos);
}

void AudioSourceClip::restart()
{
	streams[0] = {};
	streams[1] = {};
	pendingHandle.reset();
	initialised = false;
}

bool AudioSourceClip::isLooping()
{
	return looping;
}

void AudioSourceClip::prime()
{
	if (clip->hasStreamHandles() && clip->isLoaded() && !pendingHandle && !streams[0].streamingHandle) {
		requestStreamHandle(pickStartPos());
	}
}

size_t AudioSourceClip::getPrimaryEndPos() const
{
	const size_t clipLength = clip->getLength();
	return loopEnd > 0 && std::cmp_less(loopEnd, clipLength) ? static_cast<size_t>(loopEnd) : clipLength;
}

size_t AudioSourceClip::pickStartPos() const
{
	return looping && randomiseStart ? engine.getRNG().getSizeT(0, getPrimaryEndPos()) : 0;
}

void AudioSourceClip::requestStreamHandle(size_t startPos)
{
	auto pending = std::make_shared<PendingHandle>();
	pending->startPos = startPos;
	pendingHandle = pending;
	
	Executors::getDiskIO().addToQueue([weak = std::weak_ptr<PendingHandle>(pending), clipRef = clip]()
	{
		if (auto p = weak.lock()) {
			p->handle = clipRef->makeStreamHandle(p->startPos);
			p->ready.store(true, std::memory_order_release);
		}
	}, {});
}

void AudioSourceClip::takeStreamHandle()
{
	auto& stream = streams[0];
	const size_t startPos = pendingHandle->startPos;
	auto handle = std::move(pendingHandle->handle);
	pendingHandle.reset();
	
	if (randomiseStart) {
		stream.streamingHandle = std::move(handle);
		stream.playbackPos = startPos;
		return;
	}
	
	// Playback kept moving while the open was in flight. Past a second of drift, or after a loop wrap,
	// a fresh open at the current position is cheaper than decoding through it and keeps the seek off this thread.
	constexpr size_t maxCatchUp = AudioConfig::sampleRate;
	const size_t target = stream.playbackPos;
	if (target < startPos || target - startPos > maxCatchUp) {
		requestStreamHandle(target);
		return;
	}
	
	// Decode forward through whatever played as silence while the open was in flight.
	// Sequential reads are inexpensive and hit the read-ahead cache; a seek would bisect the file on this thread.
	// Past a second of drift, let the next read seek instead.
	stream.streamingHandle = std::move(handle);
	constexpr size_t step = 4096;
	for (size_t pos = startPos; pos <target; pos += step) {
		clip->prepareChannelData(pos, std::min(step, target - pos), stream.streamingHandle.get());
	}
}

bool AudioSourceClip::getAudioData(size_t samplesRequested, AudioMultiChannelSamples dstChannels)
{
	const auto trace = StackDebugTrace("audioClip", clip->getName());
	HalleyAssertDev(isReady());

	// Set stream end positions
	const auto clipLength = clip->getLength();
	const size_t loopRestartPos = std::max(static_cast<size_t>(loopStart), clip->getLoopPoint());
	streams[0].endPos = getPrimaryEndPos();
	streams[0].kickOffSecondStream = streams[0].endPos < clipLength;
	streams[1].endPos = clipLength;

	if (!initialised) {
		initialised = true;

		streams[0].active = true;
		streams[0].loop = looping;
		
		if (clip->hasStreamHandles() && !pendingHandle && !streams[0].streamingHandle) {
			requestStreamHandle(pickStartPos());
		}
		streams[0].playbackPos = pendingHandle ? pendingHandle->startPos : pickStartPos();
	}

	uint8_t nDstChannels = 0;
	for (auto& dst: dstChannels) {
		if (!dst.empty()) {
			++nDstChannels;
		} else {
			break;
		}
	}
	const uint8_t nSrcChannels = getNumberOfChannels();
	const uint8_t nChannels = std::min(nSrcChannels, nDstChannels);
	if (nSrcChannels > nDstChannels) {
		Logger::logError("AudioClip \"" + getName() + "\" has more channels (" + toString(static_cast<int>(nSrcChannels)) + ") than upstream is expecting (" + toString(static_cast<int>(nDstChannels)) + ")", true);
	}

	if (pendingHandle) {
		if (!pendingHandle->ready.load(std::memory_order_acquire)) {
			// Still opening. Silence for now, but keep time moving so we can catch up when it lands.
			AudioMixer::zeroRange(dstChannels, nChannels, 0, samplesRequested);
			streams[0].playbackPos += samplesRequested;
			if (streams[0].playbackPos >= streams[0].endPos) {
				streams[0].playbackPos = looping ? loopRestartPos : streams[0].endPos;
			}
			prevGain = gain;
			return true;
		}
		takeStreamHandle();
	}
	
	size_t samplesWritten = 0;

	while (samplesWritten < samplesRequested) {
		for (auto& stream: streams) {
			if (stream.active) {
				if (stream.playbackPos >= stream.endPos) {
					// If we're at the end of playback, either loop, or flag as done
					if (stream.loop) {
						const auto prevPos = stream.playbackPos;
						stream.playbackPos = std::max(static_cast<size_t>(loopStart), clip->getLoopPoint());
						if (stream.playbackPos >= clipLength) {
							// Loop failed
							looping = false;
							stream.playbackPos = loopRestartPos;
							stream.active = false;
						} else {
							// Loop ok
							if (stream.kickOffSecondStream) {
								streams[1].active = true;
								streams[1].playbackPos = prevPos;
							}
						}
					} else if (!clip->isLive()){
						stream.active = false;
					}
				}
			}
		}

		size_t streamsActive = 0;
		size_t samplesAvailable = std::numeric_limits<size_t>::max();
		for (const auto& stream: streams) {
			if (stream.active) {
				samplesAvailable = std::min(samplesAvailable, stream.endPos - stream.playbackPos);
				++streamsActive;
			}
		}
		if (streamsActive == 0) {
			samplesAvailable = 0;
		}

		const size_t samplesRemaining = samplesRequested - samplesWritten;
		const size_t samplesToRead = std::min(samplesRemaining, samplesAvailable);

		if (samplesToRead > 0) {
			// We have some samples that we can read, so go ahead with reading them
			bool first = true;

			for (auto& stream: streams) {
				if (stream.active) {
					if (clip->hasStreamHandles() && !stream.streamingHandle) {
						stream.streamingHandle = clip->makeStreamHandle();
					}

					clip->prepareChannelData(stream.playbackPos, samplesToRead, stream.streamingHandle.get());
					if (first) {
						for (size_t ch = 0; ch < nChannels; ++ch) {
							auto dst = dstChannels[ch].subspan(samplesWritten, samplesToRead);
							const size_t nCopied = clip->copyChannelData(ch, stream.playbackPos, samplesToRead, prevGain, gain, dst);
							HalleyAssertDev(nCopied <= samplesRequested * sizeof(AudioSample));
						}
						first = false;
					} else {
						auto buffer = engine.getPool().getBuffer(samplesToRead);
						for (size_t ch = 0; ch < nChannels; ++ch) {
							auto dst = dstChannels[ch].subspan(samplesWritten, samplesToRead);
							const size_t nCopied = clip->copyChannelData(ch, stream.playbackPos, samplesToRead, prevGain, gain, buffer.getSpan());
							AudioMixer::mixAudio(buffer.getSpan(), dst, 1, 1);
							HalleyAssertDev(nCopied <= samplesRequested * sizeof(AudioSample));
						}
					}

					stream.playbackPos += static_cast<int64_t>(samplesToRead);
				}
			}

			samplesWritten += samplesToRead;
		} else {
			// Reached end of playback, pad with zeroes
			AudioMixer::zeroRange(dstChannels, nChannels, samplesWritten, samplesRemaining);
			samplesWritten += samplesRemaining;
		}
	}

	prevGain = gain;

	return std::any_of(streams.begin(), streams.end(), [] (const auto& s) { return s.active; });
}
