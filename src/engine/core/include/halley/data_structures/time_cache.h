#pragma once

#include "hash_map.h"
#include "../utils/algorithm.h"

namespace Halley {
    template <typename K, typename V, typename Time>
    class TimeCache {
    public:
        struct Entry {
            V value;
            Time age = 0;
        };

    	template <typename F>
        V& get(const K& key, const F& make)
        {
            if (const auto iter = values.find(key);  iter != values.end()) {
                iter->second.age = 0;
                return iter->second.value;
            } else {
                auto [i, b] = values.insert({key, Entry{ make(), 0 }});
                return i->second.value;
            }
        }

        void age(Time t, Time maxAge)
        {
	        for (auto& [k, v]: values) {
                v.age += t;
	        }

            std_ex::erase_if_value(values, [&](const Entry& e) { return e.age > maxAge; });
        }

        const HashMap<K, Entry>& getHashMap() const
    	{
            return values;
    	}

        HashMap<K, Entry>& getHashMap()
    	{
            return values;
    	}

    private:
        HashMap<K, Entry> values;
    };
}
