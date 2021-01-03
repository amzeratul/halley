#include "undo_stack.h"
using namespace Halley;

void UndoStack::pushAdded(const String& parent, const EntityData& data)
{
	// TODO
}

void UndoStack::pushRemoved(const String& entityId, const EntityData& data)
{
	// TODO
}

void UndoStack::pushMoved(const String& entityId, const String& prevParent, const String& newParent)
{
	// TODO
}

void UndoStack::pushModified(const String& entityId, const EntityData& before, const EntityData& after)
{
	const auto fwdDelta = EntityDataDelta(before, after);
	const auto backDelta = EntityDataDelta(after, before);
	//Logger::logDev("Forward:\n" + EntityData(fwdDelta).toYAML() + "\n");
	//Logger::logDev("Back:\n" + EntityData(backDelta).toYAML() + "\n");
}
