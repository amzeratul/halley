#pragma once

namespace Halley {
	class UndoStack {
	public:
		void pushAdded(const String& parent, const EntityData& data);
		void pushRemoved(const String& entityId, const EntityData& data);
		void pushMoved(const String& entityId, const String& prevParent, const String& newParent);
		void pushModified(const String& entityId, const EntityData& before, const EntityData& after);

	private:
		enum class Type {
			EntityAdded,
			EntityRemoved,
			EntityMoved,
			EntityModified
		};

		class Action {
		public:
		};
	};
}
