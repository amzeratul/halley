#pragma once

#include <memory>

#include "path.h"
#include "halley/text/halleystring.h"
#include "halley/utils/hash.h"

namespace Halley
{
	class Path;
	class DirectoryMonitorPimpl;

	class DirectoryMonitor
	{
	public:
		enum class ChangeType {
			Unknown, // Assume that anything might have changed
			FileAdded,
			FileRemoved,
			FileModified,
			FileRenamed
		};

		struct Event {
			ChangeType type;
			String name;
			String oldName;

			bool operator==(const Event& other) const
			{
				return type == other.type && name == other.name && oldName == other.oldName;
			}

			bool operator!=(const Event& other) const
			{
				return !(*this == other);
			}
		};

		explicit DirectoryMonitor(const Path& p);
		~DirectoryMonitor();

		void poll(Vector<Event>& output, bool waitForNoChange = false);
		Vector<Event> poll(bool waitForNoChange = false);
		bool pollAny();

		bool hasRealImplementation() const;

	private:
		std::unique_ptr<DirectoryMonitorPimpl> pimpl;

		void postProcessEvents(Vector<Event>& events);
	};
}

namespace std {
	template <>
	struct hash<Halley::DirectoryMonitor::Event> {
	public:
		size_t operator()(const Halley::DirectoryMonitor::Event& event) const
		{
			Halley::Hash::Hasher hasher;
			hasher.feed(event.name);
			hasher.feed(event.oldName);
			hasher.feed(event.type);
			return hasher.digest();
		}
	};
}