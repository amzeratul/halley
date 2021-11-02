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

void UndoStack::pushRemoved(bool wasModified, gsl::span<const String> entityIds, gsl::span<const std::pair<String, int>> parents, gsl::span<const EntityData> datas)
{
	if (!accepting) {
		return;
	}

	std::vector<EntityPatch> forwardPatches;
	std::vector<EntityPatch> backPatches;
	forwardPatches.reserve(entityIds.size());
	backPatches.reserve(entityIds.size());

	for (size_t i = 0; i < entityIds.size(); ++i) {
		auto forward = EntityDataDelta();
		auto back = EntityDataDelta(datas[i]);
		forwardPatches.emplace_back(EntityPatch{ std::move(forward), entityIds[i] });
		backPatches.emplace_back(EntityPatch{ std::move(back), entityIds[i], parents[i].first, parents[i].second });
	}

	addToStack(Action(Type::EntityRemoved, std::move(forwardPatches)), Action(Type::EntityAdded, std::move(backPatches)), wasModified);
}

void UndoStack::pushMoved(bool wasModified, const String& entityId, const String& prevParent, int prevIndex, const String& newParent, int newIndex)
{
	if (accepting) {
		if (prevParent != newParent || prevIndex != newIndex) {
			addToStack(Action(Type::EntityMoved, EntityDataDelta(), entityId, newParent, newIndex), Action(Type::EntityMoved, EntityDataDelta(), entityId, prevParent, prevIndex), wasModified);
		}
	}
}

bool UndoStack::pushModified(bool wasModified, gsl::span<const String> entityIds, gsl::span<const EntityData*> before, gsl::span<const EntityData*> after)
{
	Expects(entityIds.size() == before.size());
	Expects(entityIds.size() == after.size());
	
	if (!accepting) {
		return true;
	}

	std::vector<EntityPatch> forwardPatches;
	std::vector<EntityPatch> backPatches;
	forwardPatches.reserve(entityIds.size());
	backPatches.reserve(entityIds.size());
	bool hasAnyChange = false;
	for (size_t i = 0; i < entityIds.size(); ++i) {
		EntityDataDelta::Options options;
		options.shallow = true;
		auto forward = EntityDataDelta(*before[i], *after[i], options);
		auto back = EntityDataDelta(*after[i], *before[i], options);
		hasAnyChange = hasAnyChange || forward.hasChange();
		forwardPatches.emplace_back(EntityPatch{ std::move(forward), entityIds[i] });
		backPatches.emplace_back(EntityPatch{ std::move(back), entityIds[i] });
	}
	
	if (hasAnyChange) {
		addToStack(Action(Type::EntityModified, std::move(forwardPatches)), Action(Type::EntityModified, std::move(backPatches)), wasModified);
		return true;
	} else {
		return false;
	}
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

UndoStack::Action::Action(Type type, std::vector<EntityPatch> patches)
	: type(type)
	, patches(std::move(patches))
{
}

bool UndoStack::ActionPair::isCompatibleWith(const Action& newForward) const
{
	if (forward.type != newForward.type || forward.patches.size() != newForward.patches.size()) {
		return false;
	}

	// Check patch compatibility
	for (size_t i = 0; i < forward.patches.size(); ++i) {
		if (!arePatchesCompatible(forward.patches[i], newForward.patches[i], forward.type)) {
			return false;
		}
	}

	return true;
}

bool UndoStack::ActionPair::arePatchesCompatible(const EntityPatch& a, const EntityPatch& b, Type type) const
{
	if (a.entityId != b.entityId) {
		return false;
	}

	if (type == Type::EntityModified) {
		return a.delta.modifiesTheSameAs(b.delta);
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
	
	switch (action.type) {
	case Type::EntityAdded:
		sceneEditorWindow.addEntities(action.patches);
		break;

	case Type::EntityRemoved:
		sceneEditorWindow.removeEntities(action.patches);
		break;
		
	case Type::EntityMoved:
		sceneEditorWindow.moveEntities(action.patches);
		break;

	case Type::EntityModified:
		sceneEditorWindow.modifyEntities(action.patches);
		break;

	case Type::EntityReplaced:
		sceneEditorWindow.replaceEntities(action.patches);
		break;
	}

	if (action.clearModified) {
		sceneEditorWindow.clearModifiedFlag();
	}
	
	accepting = true;
}
