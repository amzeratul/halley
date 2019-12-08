#pragma once

#include <halley/text/halleystring.h>

namespace Halley {
	class Path;

    class ProjectProperties {
    public:
		ProjectProperties(const Path& propertiesFile);

		const String& getName() const;
		void setName(String name);

    private:
		const Path& propertiesFile;
		String name;

		void load();
		void save();
    };
}
