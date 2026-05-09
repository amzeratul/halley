#include "string_output_stream.h"

#include <halley/api/web_api.h>

#include "halley/utils/algorithm.h"

using namespace Halley;

ConfigNode StringOutputStringState::toConfigNode() const
{
	ConfigNode result;
	result["id"] = id;
	result["type"] = type;
	result["values"] = values;
	result["language"] = language;
	result["metrics"] = metrics;
	return result;
}

StringOutputState::StringOutputState()
	: lock(mutex, std::defer_lock_t())
{
}

StringOutputState::StringOutputState(const StringOutputState& other)
	: strings(other.strings)
	, frameNumber(other.frameNumber.load())
	, lock(mutex, std::defer_lock_t())
{
}

void StringOutputState::startFrame()
{
	lock.lock();

	writingFrame = true;
	frameModified = false;

	for (auto& s: strings) {
		s.present = false;
	}
}

void StringOutputState::endFrame()
{
	writingFrame = false;
	++frameNumber;

	const bool erased = std_ex::erase_if(strings, [&] (const auto& s)
	{
		return !s.present;
	});
	if (erased) {
		frameModified = true;
	}

	if (frameModified) {
		lastNewFrameNumber = frameNumber.load();
	}

	lock.unlock();

	if (frameModified) {
		condition.notifyAll();
	}
}

void StringOutputState::reportString(const String& id, StringOutputType type, const LocalisedString& string, const StringOutputMetrics& metrics, const I18N& i18n)
{
	if (string.getString().isEmpty()) {
		return;
	}

	const auto& lang = string.getLanguage(i18n);

	StringOutputStringState str;
	str.id = id;
	str.type = type;
	str.values[lang.getISOCode()] = string.getString();
	str.metrics = metrics;
	str.language = lang.getISOCode();

	if (const auto& secondary = i18n.getSecondaryLanguage(); secondary && secondary != lang) {
		str.values[secondary->getISOCode()] = string.replaceLanguage(*secondary).getString();
	}

	auto& curStr = getString(id);
	curStr.present = true;

	if (curStr != str) {
		curStr = std::move(str);
		frameModified = true;
	}
}

StringOutputStringState& StringOutputState::getString(const String& id)
{
	for (auto& s: strings) {
		if (s.id == id) {
			return s;
		}
	}

	auto& s = strings.emplace_back();
	s.id = id;
	return s;
}

StringOutputStream::StringOutputStream(const StringOutputState& srcState)
	: srcState(srcState)
	, lastFrameSeen(0)
{
	updateStrings();
}

void StringOutputStream::waitAndUpdateState()
{
	using namespace std::chrono_literals;

	while (true) {
		auto lock = UniqueLock<Mutex>(srcState.mutex);
		srcState.condition.waitFor(lock, 100ms);

		if (srcState.lastNewFrameNumber.load() > lastFrameSeen) {
			updateStrings();
			return;
		}
	}
}

void StringOutputStream::outputDelta(HTTPServerDataSink& sink)
{
	for (auto& event: outputEvents) {
		sink.write("event: " + event.type + "\ndata: " + JSONConvert::generateJSON(event.data) + "\n\n");
	}
	outputEvents.clear();
}

void StringOutputStream::updateStrings()
{
	HashSet<String> existingIds;

	const auto& srcStrs = srcState.strings;
	for (const auto& src: srcStrs) {
		existingIds.emplace(src.id);

		bool exists = false;
		auto& dst = getString(src.id, exists);
		if (src != dst) {
			dst = src;
			outputEvents += makeChangeEvent(dst, !exists);
		}
	}

	std_ex::erase_if(strings, [&] (const StringOutputStringState& s)
	{
		if (!existingIds.contains(s.id)) {
			outputEvents += makeRemoveEvent(s);
			return true;
		}
		return false;
	});

	lastFrameSeen = srcState.frameNumber;
}

StringOutputStringState& StringOutputStream::getString(const String& id, bool& exists)
{
	for (auto& s: strings) {
		if (s.id == id) {
			exists = true;
			return s;
		}
	}
	exists = false;
	return strings.emplace_back();
}

StringOutputStream::Event StringOutputStream::makeChangeEvent(const StringOutputStringState& str, bool isNew) const
{
	Event result;
	result.type = isNew ? "addString" : "updateString";
	result.data = str.toConfigNode();
	return result;
}

StringOutputStream::Event StringOutputStream::makeRemoveEvent(const StringOutputStringState& str) const
{
	Event result;
	result.type = "removeString";
	ConfigNode data;
	data["id"] = str.id;
	result.data = std::move(data);
	return result;
}
