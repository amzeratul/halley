#include "undo_stack.h"

#include "scene_editor_window.h"
using namespace Halley;

bool UndoStack::EntityPatch::isCompatibleWith(const EntityPatch& other, Type type) const
{
	if (entityId != other.entityId) {
		return false;
	}

	if (type == Type::EntityModified) {
		return delta.modifiesTheSameAs(other.delta);
	}

	return true;
}

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

bool UndoStack::pushReplaced(bool wasModified, const String& entityId, const EntityData& before, const EntityData& after)
{
	if (accepting) {
		auto forward = EntityDataDelta(after);
		auto back = EntityDataDelta(before);
		addToStack(Action(Type::EntityReplaced, std::move(forward), entityId), Action(Type::EntityReplaced, std::move(back), entityId), wasModified);
		return true;
	}
	return true;
}

void UndoStack::undo(SceneEditorWindow& sceneEditorWindow)
{
	if (canUndo()) {
		runAction(stack[--stackPos]->back, sceneEditorWindow);
	}
}

void UndoStack::redo(SceneEditorWindow& sceneEditorWindow)
{
	if (canRedo()) {
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

bool UndoStack::canUndo() const
{
	return stackPos > 0;
}

bool UndoStack::canRedo() const
{
	return stackPos < stack.size();
}

UndoStack::Action::Action(Type type, EntityDataDelta delta, String entityId, String parent, int childIndex)
	: type(type)
{
	auto& a = patches.emplace_back();
	a.delta = std::move(delta);
	a.entityId = std::move(entityId);
	a.parent = std::move(parent);
	a.childIndex = childIndex;
}

bool UndoStack::ActionPair::isCompatibleWith(const Action& newForward) const
{
	if (forward.type != newForward.type || forward.patches.size() != newForward.patches.size()) {
		return false;
	}

	// Check patch compatibility
	for (size_t i = 0; i < forward.patches.size(); ++i) {
		if (!forward.patches[i].isCompatibleWith(newForward.patches[i], forward.type)) {
			return false;
		}
	}

	return true;
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

	auto& patch = action.patches.front(); // HACK

	switch (action.type) {
	case Type::EntityAdded:
		sceneEditorWindow.addEntity(patch.parent, patch.childIndex, EntityData(patch.delta));
		break;

	case Type::EntityRemoved:
		sceneEditorWindow.removeEntity(patch.entityId);
		break;
		
	case Type::EntityMoved:
		sceneEditorWindow.moveEntity(patch.entityId, patch.parent, patch.childIndex);
		break;

	case Type::EntityModified:
		sceneEditorWindow.modifyEntity(patch.entityId, patch.delta);
		break;

	case Type::EntityReplaced:
		sceneEditorWindow.replaceEntity(patch.entityId, EntityData(patch.delta));
		break;
	}

	if (action.clearModified) {
		sceneEditorWindow.clearModifiedFlag();
	}
	
	accepting = true;
}
