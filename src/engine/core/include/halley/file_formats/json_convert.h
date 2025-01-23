#pragma once

#include <gsl/gsl>

#include "halley/file_formats/config_file.h"

namespace Halley {
    class JSONConvert {
    public:
        struct Options {
	        bool compact = true;
        };

        [[nodiscard]] static ConfigNode parseConfig(gsl::span<const gsl::byte> data);
        [[nodiscard]] static ConfigNode parseConfig(const Bytes& data);
        [[nodiscard]] static ConfigNode parseConfig(const String& str);

        [[nodiscard]] static String generateJSON(const ConfigNode& config, const Options& options = {});
    };
}
