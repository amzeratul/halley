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

#include "resource_locator.h"

namespace Halley {
	class FileSystemResourceLocator : public IResourceLocatorProvider {
	public:
		FileSystemResourceLocator(String basePath);

	protected:
		std::unique_ptr<ResourceData> doGet(String resource, bool stream) override;
		std::time_t doGetTimestamp(String resource) override;
		std::vector<String> getResourceList() override;
		int getPriority() override { return -1; }

		String basePath;
	};
}
