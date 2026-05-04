#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/text/string_output_server.h"

namespace Halley {
	class HTTPServerDataSink;
}

namespace Halley {
	class StringOutputStringState {
	public:
		StringOutputType type;
		String id;
		HashMap<String, String> values;
		String language;
		StringOutputMetrics metrics;

		bool present = true;

		bool operator==(const StringOutputStringState& other) const = default;
		bool operator!=(const StringOutputStringState& other) const = default;

		ConfigNode toConfigNode() const;
	};

	class StringOutputState {
	public:
		Vector<StringOutputStringState> strings;

		std::atomic<int> frameNumber;
		std::atomic<int> lastNewFrameNumber;

		mutable Mutex mutex;
		mutable ConditionVariable condition;

		StringOutputState();
		StringOutputState(const StringOutputState& other);

		void startFrame();
		void endFrame();
		void reportString(const String& id, StringOutputType type, const LocalisedString& string, const StringOutputMetrics& metrics, const I18N& i18n);

	private:
		bool writingFrame = false;
		bool frameModified = false;

		UniqueLock<Mutex> lock;

		StringOutputStringState& getString(const String& id);
	};

	enum class StringOutputEventType {
		Add,
		Remove,
		Modify
	};

	class StringOutputStream {
	public:
		StringOutputStream(const StringOutputState& srcState);

		void waitAndUpdateState();
		void outputDelta(HTTPServerDataSink& sink);

	private:
		struct Event {
			String type;
			ConfigNode data;
		};

		const StringOutputState& srcState;
		Vector<StringOutputStringState> strings;
		Vector<Event> outputEvents;
		int lastFrameSeen = -1;

		void updateStrings();
		StringOutputStringState& getString(const String& id, bool& exists);

		Event makeChangeEvent(const StringOutputStringState& str, bool isNew) const;
		Event makeRemoveEvent(const StringOutputStringState& str) const;
	};
}
