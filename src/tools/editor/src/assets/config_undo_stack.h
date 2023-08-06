#pragma once

#include "halley/data_structures/config_node.h"

namespace Halley {
    class ConfigUndoStack {
    public:
        ConfigUndoStack(size_t maxLevels);

        void clear();
        void loadInitialValue(const ConfigNode& curData);
        void update(const ConfigNode& curData);
        void startAction();
        void onSave();

        const ConfigNode& undo();
        const ConfigNode& redo();

        bool canUndo() const;
        bool canRedo() const;

    private:
        size_t maxLevels;
        Vector<ConfigNode> stack;
        size_t curPos = 0;
        bool acceptingDelta = false;

        bool canMerge(const ConfigNode& curData) const;
    };
}
