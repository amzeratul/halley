#pragma once
#include "halley/time/halleytime.h"

namespace Halley {
	class Deserializer;
	class Serializer;

	class SharedData {
    public:
		virtual ~SharedData() = default;

		void markModified();
		void markUnmodified();
		bool isModified() const;

		Time getTimeSinceLastSend() const;
		void markSent();
		void update(Time t);

		virtual void serialize(Serializer& s) const;
		virtual void deserialize(Deserializer& s);

	private:
		bool modified = false;
		Time timeSinceLastSend = 0.0;
    };
}
