#include "halley/diagnostics/audio_view.h"

#include "halley/api/halley_api.h"
#include "halley/audio/audio_object.h"
#include "halley/entity/world.h"
#include "halley/graphics/painter.h"

using namespace Halley;

AudioView::AudioView(Resources& resources, const HalleyAPI& api)
	: StatsView(resources, api)
	, boxBg(Sprite().setImage(resources, "halley/box_2px_outline.png"))
	, whitebox(Sprite().setImage(resources, "whitebox.png"))
{
	headerText = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f));
}

AudioView::~AudioView()
{
	if (listenerRegistered) {
		listenerRegistered = false;
		api.audio->setDebugListener(nullptr);
	}
}

void AudioView::update(Time t)
{
	if (active != listenerRegistered) {
		if (active) {
			api.audio->setDebugListener(this);
			listenerRegistered = true;
		} else {
			api.audio->setDebugListener(nullptr);
			listenerRegistered = false;
		}
	}

	if (active && !populatedObjectNames) {
		populatedObjectNames = true;
		for (const auto& assetId: resources.enumerate<AudioObject>()) {
			objectNames[resources.get<AudioObject>(assetId)->getAudioObjectId()] = assetId;
		}
	}
}

void AudioView::paint(Painter& painter)
{
	if (!active) {
		return;
	}

	{
		std::unique_lock<std::mutex> lock(mutex);
		curData = lastData;
	}

	const auto rect = Rect4f(painter.getViewPort());
	whitebox.clone().setPosition(Vector2f(0, 0)).scaleTo(Vector2f(rect.getSize())).setColour(Colour4f(0, 0, 0, 0.5f)).draw(painter);

	Vector2f textPos = Vector2f(10, 90);

	Colour4f keyCol(0.75f, 0.75f, 0.75f);
	Colour4f valueCol(0.5f, 0.5f, 0.5f);

	{
		ColourStringBuilder str;

		str.append("Listener at regions:");
		for (const auto& region: curData.listener.regions) {
			str.append("\n- ");
			str.append(getRegionName(region.regionId), keyCol);
			str.append(" with presence ");
			str.append(toString(region.presence, 2), valueCol);
		}

		auto results = str.moveResults();
		headerText
			.setPosition(textPos)
			.setText(results.first)
			.setColourOverride(results.second)
			.draw(painter);
		auto extents = headerText.getExtents();
		textPos.y += extents.y + 16;
	}

	std::sort(curData.emitters.begin(), curData.emitters.end(), [=] (const auto& a, const auto& b)
	{
		return a.emitterId < b.emitterId;
	});

	for (auto& emitterData: curData.emitters) {
		if (emitterData.voices.empty()) {
			continue;
		}

		ColourStringBuilder str;

		str.append(getEmitterName(emitterData.emitterId));
		str.append(" at region ");
		str.append(getRegionName(emitterData.regionId), valueCol);

		bool first = true;
		for (const auto& [k, v]: emitterData.switches) {
			if (first) {
				str.append("\n    ");
				first = false;
			} else {
				str.append(", ", keyCol);
			}
			str.append(k, keyCol);
			str.append(": ", keyCol);
			str.append(v, valueCol);
		}

		first = true;
		for (const auto& [k, v]: emitterData.variables) {
			if (first) {
				str.append("\n    ");
				first = false;
			} else {
				str.append(", ", keyCol);
			}
			str.append(k, keyCol);
			str.append(": ", keyCol);
			str.append(toString(v, 2), valueCol);
		}

		for (auto& voiceData: emitterData.voices) {
			str.append("\n- ");
			str.append(getObjectName(voiceData.objectId), keyCol);
			str.append(": gain = ");
			str.append(toString(voiceData.gain), valueCol);
			str.append(", pause = ");
			str.append(toString(voiceData.paused), valueCol);
			str.append(", mix = ");
			str.append("[" + String::concat(gsl::span<const float>(voiceData.channelMix).subspan(0, voiceData.dstChannels), ", ", [](float v) { return toString(v, 2); }) + "]", valueCol);
		}

		auto results = str.moveResults();
		headerText
			.setPosition(textPos)
			.setText(results.first)
			.setColourOverride(results.second)
			.draw(painter);
		auto extents = headerText.getExtents();
		textPos.y += extents.y + 16;
	}
}

void AudioView::onAudioDebugData(AudioDebugData data)
{
	std::unique_lock<std::mutex> lock(mutex);
	lastData = std::move(data);
}

String AudioView::getEmitterName(AudioEmitterId emitterId) const
{
	if (emitterId == 0) {
		return "Global Singleton";
	}

	const auto iter = emitterNames.find(emitterId);
	if (iter != emitterNames.end()) {
		return iter->second;
	}
	auto name = world->getInterface<IAudioSystemInterface>().getSourceName(emitterId);
	emitterNames[emitterId] = name;
	return name;
}

String AudioView::getRegionName(AudioRegionId regionId) const
{
	if (regionId == 0) {
		return "Global Region (0)";
	}

	const auto iter = regionNames.find(regionId);
	if (iter != regionNames.end()) {
		return iter->second;
	}
	auto name = world->getInterface<IAudioSystemInterface>().getRegionName(regionId) + " (" + toString(int(regionId)) + ")";
	regionNames[regionId] = name;
	return name;
}

String AudioView::getObjectName(AudioObjectId objectId) const
{
	const auto iter = objectNames.find(objectId);
	if (iter != objectNames.end()) {
		return iter->second;
	}
	return "<unknown>";
}
