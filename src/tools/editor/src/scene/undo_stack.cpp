#include "undo_stack.h"

#include "scene_editor_window.h"
using namespace Halley;

UndoStack::UndoStack()
	: maxSize(50)
	, accepting(true)
{
}

void UndoStack::pushAdded(bool wasModified, const String& entityId, const String& parent, int childIndex, const EntityData& data)
{
	if (accepting) {
		addToStack(Action(Type::EntityAdded, EntityDataDelta(data), entityId, parent, childIndex), Action(Type::EntityRemoved, EntityDataDelta(), entityId), wasModified);
	}
}

void UndoStack::pushRemoved(bool wasModified, const String& entityId, const String& parent, int childIndex, const EntityData& data)
{
	if (accepting) {
		addToStack(Action(Type::EntityRemoved, EntityDataDelta(), entityId), Action(Type::EntityAdded, EntityDataDelta(data), entityId, parent, childIndex), wasModified);
	}
}

void UndoStack::pushMoved(bool wasModified, const String& entityId, const String& prevParent, int prevIndex, const String& newParent, int newIndex)
{
	if (accepting) {
		if (prevParent != newParent || prevIndex != newIndex) {
			addToStack(Action(Type::EntityMoved, EntityDataDelta(), entityId, newParent, newIndex), Action(Type::EntityMoved, EntityDataDelta(), entityId, prevParent, prevIndex), wasModified);
		}
	}
}

bool UndoStack::pushModified(bool wasModified, const String& entityId, const EntityData& before, const EntityData& after)
{
	if (accepting) {
		auto forward = EntityDataDelta(before, after);
		if (forward.hasChange()) {
			auto back = EntityDataDelta(after, before);
			addToStack(Action(Type::EntityModified, std::move(forward), entityId), Action(Type::EntityModified, std::move(back), entityId), wasModified);
			return true;
		} else {
			return false;
		}
	}
	return true;
}

void UndoStack::undo(SceneEditorWindow& sceneEditorWindow)
{
	if (stackPos > 0) {
		runAction(stack[--stackPos]->back, sceneEditorWindow);
	}
}

void UndoStack::redo(SceneEditorWindow& sceneEditorWindow)
{
	if (stackPos < stack.size()) {
		runAction(stack[stackPos++]->forward, sceneEditorWindow);
	}
}

void UndoStack::onSave()
{
	// When saving, reset everyone's clear flag, unless they're the action that will lead back here
	for (size_t i = 0; i < stack.size(); ++i) {
		auto& a = stack[i];
		a->back.clearModified = i == stackPos;
		a->forward.clearModified = i + 1 == stackPos;
	}
}

bool UndoStack::ActionPair::isCompatibleWith(const Action& newForward) const
{
	if (forward.type != newForward.type || forward.entityId != newForward.entityId) {
		return false;
	}

	if (forward.type == Type::EntityModified) {
		return forward.delta.modifiesTheSameAs(newForward.delta);
	}
	
	return false;
}

void UndoStack::addToStack(Action forward, Action back, bool wasModified)
{
	// Discard redo timeline that is no longer valid
	const bool hadRedo = stack.size() > stackPos;
	if (hadRedo) {
		stack.resize(stackPos);
	}

	if (!hadRedo && !stack.empty() && stack.back()->isCompatibleWith(forward)) {
		// User is doing more of the same action, combine them instead
		stack.back()->forward = std::move(forward);
	} else {
		// Insert new action into stack
		stack.emplace_back(std::make_unique<ActionPair>(std::move(forward), std::move(back)));
		stack.back()->back.clearModified = !wasModified;
		if (stack.size() > maxSize) {
			stack.erase(stack.begin());
		}
		stackPos = stack.size();
	}
}

void UndoStack::runAction(const Action& action, SceneEditorWindow& sceneEditorWindow)
{
	accepting = false;

	switch (action.type) {
	case Type::EntityAdded:
		sceneEditorWindow.addEntity(action.parent, action.childIndex, EntityData(action.delta));
		break;

	case Type::EntityRemoved:
		sceneEditorWindow.removeEntity(action.entityId);
		break;
		
	case Type::EntityMoved:
		sceneEditorWindow.moveEntity(action.entityId, action.parent, action.childIndex);
		break;

	case Type::EntityModified:
		sceneEditorWindow.modifyEntity(action.entityId, action.delta);
		break;
	}

	if (action.clearModified) {
		sceneEditorWindow.clearModifiedFlag();
	}
	
	accepting = true;
}
