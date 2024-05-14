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

	const auto rect = Rect4f(painter.getViewPort());
	whitebox.clone().setPosition(Vector2f(0, 0)).scaleTo(Vector2f(rect.getSize())).setColour(Colour4f(0, 0, 0, 0.5f)).draw(painter);

	Vector2f emitterPos = Vector2f(10, 90);

	Colour4f keyCol(0.75f, 0.75f, 0.75f);
	Colour4f valueCol(0.5f, 0.5f, 0.5f);

	std::sort(lastData.emitters.begin(), lastData.emitters.end(), [=] (const auto& a, const auto& b)
	{
		return a.emitterId < b.emitterId;
	});

	for (auto& emitterData: lastData.emitters) {
		if (emitterData.voices.empty()) {
			continue;
		}

		ColourStringBuilder str;

		str.append(getName(emitterData.emitterId));

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
			str.append(toString(v), valueCol);
		}

		for (auto& voiceData: emitterData.voices) {
			str.append("\n- ");
			str.append(getObjectName(voiceData.objectId), keyCol);
			str.append(": gain = ");
			str.append(toString(voiceData.gain), valueCol);
			str.append(", pause = ");
			str.append(toString(voiceData.paused), valueCol);
		}

		auto results = str.moveResults();
		headerText
			.setPosition(emitterPos)
			.setText(results.first)
			.setColourOverride(results.second)
			.draw(painter);
		auto extents = headerText.getExtents();
		emitterPos.y += extents.y + 16;
	}
}

void AudioView::onAudioDebugData(AudioDebugData data)
{
	lastData = data;
}

String AudioView::getName(AudioEmitterId emitterId) const
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

String AudioView::getObjectName(AudioObjectId objectId) const
{
	const auto iter = objectNames.find(objectId);
	if (iter != objectNames.end()) {
		return iter->second;
	}
	return "<unknown>";
}
