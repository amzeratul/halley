#pragma once

namespace Halley {
	class Deserializer;
	class Serializer;

	class SharedData {
    public:
		virtual ~SharedData() = default;

		void markModified();
		void markUnmodified();
		bool isModified() const;

		virtual void serialize(Serializer& s) const = 0;
		virtual void deserialize(Deserializer& s) = 0;

	private:
		bool modified = false;
    };
}
