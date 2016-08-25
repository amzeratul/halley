#pragma once

#include "prec.h"

namespace Halley
{
	class Preferences
	{
	public:
		explicit Preferences(String path);
		void addRecent(String path);
		Rect4i getWindowRect() const;

		void save() const;
		void load();

	private:
		String filePath;
	};
}
