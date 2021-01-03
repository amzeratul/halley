#pragma once

namespace Halley {
	class SceneEditorWindow;

	class UndoStack {
	public:
		UndoStack();
		
		void pushAdded(const String& entityId, const String& parent, int childIndex, const EntityData& data);
		void pushRemoved(const String& entityId, const String& parent, int childIndex, const EntityData& data);
		void pushMoved(const String& entityId, const String& prevParent, int prevIndex, const String& newParent, int newIndex);
		void pushModified(const String& entityId, const EntityData& before, const EntityData& after);

		void undo(SceneEditorWindow& sceneEditorWindow);
		void redo(SceneEditorWindow& sceneEditorWindow);

	private:
		enum class Type {
			EntityAdded,
			EntityRemoved,
			EntityMoved,
			EntityModified
		};

		class Action {
		public:
			Type type;
			EntityDataDelta delta;
			String entityId;
			String parent;
			int childIndex;

			Action() = default;
			Action(Type type, EntityDataDelta delta, String entityId, String parent = "", int childIndex = -1)
				: type(type)
				, delta(std::move(delta))
				, entityId(std::move(entityId))
				, parent(std::move(parent))
				, childIndex(childIndex)
			{}
		};

		struct ActionPair {
		public:
			Action forward;
			Action back;

			ActionPair() = default;
			ActionPair(Action forward, Action back) : forward(std::move(forward)), back(std::move(back)) {}
		};

		std::vector<std::unique_ptr<ActionPair>> stack;
		size_t stackPos = 0;
		const size_t maxSize;
		bool accepting;

		void addToStack(Action forward, Action back);
		void runAction(const Action& action, SceneEditorWindow& sceneEditorWindow);
	};
}
