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

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "xml_forward.h"
#include <memory>
#include "../text/halleystring.h"

namespace Halley {
	class XMLFile {
	public:
		XMLFile(String filename);
		XmlElement* getRoot() const;
		String getFileName() const { return filename; }

		int getNumberFlushes() const { return nFlushes; }

	private:
		void load();

		std::shared_ptr<ticpp::Document> doc;
		String filename;
		int nFlushes;
	};
}
