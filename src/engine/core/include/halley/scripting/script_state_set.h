#pragma once

#include "halley/data_structures/vector.h"
#include "halley/entity/world.h"

namespace Halley {
	class ScriptVariables;
}

namespace Halley {
	class ScriptEnvironment;
}

namespace Halley {
    class ScriptState;

    class ScriptStateSet {
    public:
        class Iterator {
        public:
            Iterator();
            Iterator(const ScriptStateSet& parent, size_t idx)
	            : parent(&parent)
				, idx(idx)
            {}

            bool operator==(const Iterator& other) const
            {
                return idx == other.idx;
            }

            bool operator!=(const Iterator& other) const
            {
                return idx != other.idx;
            }

            bool operator<(const Iterator& other) const
            {
                return idx < other.idx;
            }

            const std::shared_ptr<ScriptState>& operator*() const
            {
                return parent->getState(idx);
            }

            Iterator& operator++()
            {
                ++idx;
                return *this;
            }

            Iterator operator++(int) const
            {
                return Iterator(*parent, idx + 1);
            }

        private:
            const ScriptStateSet* parent = nullptr;
            size_t idx = 0;
        };

        void load(const ConfigNode& node, const EntitySerializationContext& context);
        ConfigNode toConfigNode(const EntitySerializationContext& context) const;

    	void addState(std::shared_ptr<ScriptState> state);
        void clear();
        bool empty() const;

        void removeDeadLocalStates(World& world, EntityId entityId);
        void terminateMarkedDead(ScriptEnvironment& environment, EntityId entityId, ScriptVariables& entityVariables);

        const std::shared_ptr<ScriptState>& getState(size_t idx) const;

    	Iterator begin() const;
        Iterator end() const;

    private:
        struct State {
            int64_t id;
            std::shared_ptr<ScriptState> state;
            bool present = true;
            bool dead = false;
        };
        Vector<State> states;
        int64_t curId = 0;

        State& getStateData(int64_t id);
        bool isValid() const;
    };

    template<>
	class ConfigNodeSerializer<ScriptStateSet> {
	public:
		ConfigNode serialize(const ScriptStateSet& stateSet, const EntitySerializationContext& context);
		ScriptStateSet deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptStateSet& target);
	};
}
