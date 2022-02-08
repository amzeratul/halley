#pragma once

#include "iserialization_dictionary.h"
#include <optional>
#include <vector>
#include <gsl/span>
#include "halley/text/halleystring.h"
#include "halley/data_structures/hash_map.h"

namespace Halley {
    class ConfigNode;
    
    class SerializationDictionary : public ISerializationDictionary {
    public:
        SerializationDictionary();
        SerializationDictionary(const ConfigNode& config);
        
        std::optional<size_t> stringToIndex(const String& string) override;
        const String& indexToString(size_t index) override;

        void addEntry(String str);
        void addEntry(size_t idx, String str);
        void addEntries(gsl::span<const String> strings);

        void setLogMissingStrings(bool enabled, int minMissing, int freq);
        void notifyMissingString(const String& string) override;
    
    private:
        std::vector<String> strings;
        HashMap<String, int> indices;
    	
        HashMap<String, int> missing;
        int missingMin = 1;
        int missingFreq = 10;
        bool logMissingStrings = false;
    };
}
