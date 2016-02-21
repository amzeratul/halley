/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2014 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "json_forward.h"
#include <memory>
#include "../text/halleystring.h"

namespace Halley {
	class JSONFile {
	public:
		JSONFile(String filename);
		JSONValue& getRoot();
		String getFileName() const { return filename; }

		int getNumberFlushes() const { return nFlushes; }

	private:
		void load();

		std::unique_ptr<JSONValue> root;
		String filename;
		int nFlushes;
	};
}
