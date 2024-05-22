#pragma once

#include "hash_map.h"
#include "vector.h"
#include "config_node.h"
#include "../text/halleystring.h"
#include <typeinfo>

namespace Halley {
    class ConfigNode;
    class ConfigFile;
    class ConfigObserver;
    class Resources;

    class IConfigDatabaseType {
    public:
        IConfigDatabaseType(String key)
            : key(std::move(key))
        {}

        virtual ~IConfigDatabaseType() = default;

        const String& getKey()
        {
            return key;
        }

        virtual void loadConfigs(const ConfigNode& nodes, bool enforceUnique) = 0;
        virtual size_t getMemoryUsage() const = 0;

    private:
        String key;
    };

    template <typename T>
    class ConfigDatabaseType : public IConfigDatabaseType {
    public:
        ConfigDatabaseType(String key)
            : IConfigDatabaseType(std::move(key))
        {}

        const T& get(std::string_view id) const
        {
            const auto iter = entries.find(id);
            if (iter != entries.end()) {
                return iter->second;
            } else {
                throw Exception(String("Entry not found in ConfigDatabaseType<") + typeid(T).name() + ">: \"" + id + "\"", HalleyExceptions::Utils);
            }
        }

        const T* tryGet(std::string_view id) const
        {
            const auto iter = entries.find(id);
            if (iter != entries.end()) {
                return &iter->second;
            }
            return nullptr;
        }

        bool contains(std::string_view id) const
        {
            return entries.contains(id);
        }

        Vector<String> getKeys() const
        {
            if (entries.empty()) {
                return {};
            }
            if (keys.empty()) {
                // Avoid threading issues
				Vector<String> keysLocal;
				keysLocal.reserve(entries.size());
				for (const auto& e: entries) {
				    keysLocal.push_back(e.first);
				}
				keys = std::move(keysLocal);
            }

            return keys;
        }

        Vector<String> getSortedKeys() const
        {
            auto keys = getKeys();
            std::sort(keys.begin(), keys.end());
            return keys;
        }

        Vector<const T*> getValues() const
        {
            Vector<const T*> result;
            result.reserve(entries.size());
            for (const auto& [k, v]: entries) {
                result.push_back(&v);
            }
            return result;
        }

        const HashMap<String, T>& getEntries() const
        {
            return entries;
        }

        HashMap<String, T>& getEntries()
        {
            return entries;
        }

        void loadConfigs(const ConfigNode& nodes, bool enforceUnique) override
        {
            if (nodes.getType() == ConfigNodeType::Sequence) {
                for (const auto& n : nodes.asSequence()) {
                    loadConfig(n, enforceUnique);
                }
            }
            keys.clear();
        }

        void loadConfig(const ConfigNode& node, bool enforceUnique)
        {
            auto id = node["id"].asString();
            if (enforceUnique && entries.contains(id)) {
	            Logger::logError("Duplicate " + String(typeid(T).name()) + " id \"" + id + "\"");
                return;
            }

            entries[std::move(id)] = T(node);
            keys.clear();
        }

        static size_t& getIdx()
        {
        	static size_t idx = std::numeric_limits<size_t>::max();
        	return idx;
        }

        size_t getMemoryUsage() const override
        {
            return sizeof(keys) + sizeof(entries) + keys.size() * sizeof(String) + entries.size_bytes();
        }

    private:
        HashMap<String, T> entries;
        mutable Vector<String> keys;
    };

    class ConfigDatabase {
    public:
        ConfigDatabase(std::optional<Vector<String>> onlyLoad = std::nullopt);

        void load(Resources& resources, const String& prefix);
        void loadFile(const ConfigFile& configFile);
        void loadConfig(const ConfigNode& node, bool enforceUnique);
        void update();

        template <typename T>
        void init(String key)
        {
            auto& idx = ConfigDatabaseType<T>::getIdx();
            if (idx == std::numeric_limits<size_t>::max()) {
                idx = nextIdx++;
            }
            dbs.reserve(std::max(dbs.size(), nextPowerOf2(idx + 1)));
        	dbs.resize(std::max(dbs.size(), idx + 1));
            dbs[idx] = std::make_unique<ConfigDatabaseType<T>>(std::move(key));
        }

        template <typename T>
        const T& get(std::string_view id) const
        {
            return of<T>().get(id);
        }

        template <typename T>
        const T* tryGet(std::string_view id) const
        {
            return of<T>().tryGet(id);
        }

        template <typename T>
        bool contains(std::string_view id) const
        {
            return of<T>().contains(id);
        }

        template <typename T>
        Vector<String> getKeys() const
        {
            return of<T>().getKeys();
        }

        template <typename T>
        Vector<String> getSortedKeys() const
        {
            return of<T>().getSortedKeys();
        }

        template <typename T>
        Vector<const T*> getValues() const
        {
            return of<T>().getValues();
        }

        template <typename T>
        const HashMap<String, T>& getEntries() const
        {
            return of<T>().getEntries();
        }

        int getVersion() const;
    	void generateMemoryReport();

    private:
        Vector<std::unique_ptr<IConfigDatabaseType>> dbs;
        HashMap<String, ConfigObserver> observers;
        int version = 0;
        bool allowHotReload = true;
        static size_t nextIdx;

        std::optional<Vector<String>> onlyLoad;

        template <typename T>
        ConfigDatabaseType<T>& of() const
        {
            return static_cast<ConfigDatabaseType<T>&>(*dbs[ConfigDatabaseType<T>::getIdx()]);
        }
    };
}
