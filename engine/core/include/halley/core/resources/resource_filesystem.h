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
	class SystemAPI;

	class FileSystemResourceLocator : public IResourceLocatorProvider {
	public:
		FileSystemResourceLocator(SystemAPI& system, Path basePath);

	protected:
		std::unique_ptr<ResourceData> doGet(const String& resource, bool stream) override;
		Vector<String> getResourceList() override;
		int getPriority() const override { return -1; }

		SystemAPI& system;
		Path basePath;
	};
}
