#include "config_undo_stack.h"
using namespace Halley;

// Stack is laid out like:
//
// U U U U R R R
//         ^- curPos
//
// With curPos pointing to the first "redo" stack frame

ConfigUndoStack::ConfigUndoStack(size_t maxLevels)
	: maxLevels(maxLevels)
{
}

void ConfigUndoStack::clear()
{
	stack.clear();
	curPos = 0;
	acceptingDelta = false;
}

void ConfigUndoStack::loadInitialValue(const ConfigNode& curData)
{
	clear();
	update(curData);
}

void ConfigUndoStack::update(const ConfigNode& curData)
{
	stack.resize(std::min(stack.size(), curPos + 1)); // Discard redo

	if (canMerge(curData)) {
		stack[curPos - 1] = curData;
	} else {
		stack.push_back(curData);
		curPos = stack.size() - 1;
		acceptingDelta = true;

		if (stack.size() > maxLevels) {
			stack.erase(stack.begin());
			--curPos;
		}
	}
}

void ConfigUndoStack::startAction()
{
	acceptingDelta = false;
}

void ConfigUndoStack::onSave()
{
	// TODO
}

const ConfigNode& ConfigUndoStack::undo()
{
	if (!canUndo()) {
		throw Exception("End of undo stack", HalleyExceptions::Utils);
	}
	return stack[--curPos];
}

const ConfigNode& ConfigUndoStack::redo()
{
	if (!canRedo()) {
		throw Exception("End of redo stack", HalleyExceptions::Utils);
	}
	return stack[++curPos];
}

bool ConfigUndoStack::canUndo() const
{
	return curPos > 0;
}

bool ConfigUndoStack::canRedo() const
{
	return curPos < stack.size() - 1;
}

bool ConfigUndoStack::canMerge(const ConfigNode& curData) const
{
	// TODO: compare if they're mergeable
	return false;
}
