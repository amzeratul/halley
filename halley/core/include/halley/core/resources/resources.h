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

#include <map>
#include <ctime>
#include <algorithm>
#include <halley/support/exception.h>
#include "resource_collection.h"

namespace Halley {
	
	class ResourceLocator;
	class HalleyAPI;
	
	template <typename T>
	class ResourceTypeId
	{
	public:
		static int getId(const std::map<std::string, int>& ids)
		{
			static int id = -1;
			if (id == -1) {
				std::string name = typeid(T).name();
				auto iter = ids.find(name);
				if (iter != ids.end()) {
					id = iter->second;
				} else {
					throw Exception("Type " + String(typeid(T).name()) + " has not been initialized.");
				}
			}
			return id;
		}

		static int makeId(std::map<std::string, int>& ids)
		{
			std::string name = typeid(T).name();
			auto iter = ids.find(name);
			if (iter != ids.end()) {
				throw Exception("Type " + String(typeid(T).name()) + " has already been initialized.");
			}

			int id = int(ids.size());
			ids[name] = id;
			return id;
		}
	};

	class Resources {
		friend class ResourceCollectionBase;

	public:
		Resources(std::unique_ptr<ResourceLocator> locator, HalleyAPI* api);
		~Resources();

		template <typename T>
		void init(String path)
		{
			int id = ResourceTypeId<T>::makeId(resourceTypeIds);
			resources.resize(std::max(resources.size(), size_t(id + 1)));
			resources[id] = std::make_unique<ResourceCollection<T>>(*this, path);
		}

		template <typename T>
		ResourceCollection<T>& of()
		{
			return static_cast<ResourceCollection<T>&>(*resources.at(ResourceTypeId<T>::getId(resourceTypeIds)));
		}

		template <typename T>
		std::shared_ptr<T> get(String name, ResourceLoadPriority priority = ResourceLoadPriority::Normal)
		{
			return of<T>().get(name, priority);
		}

		void setBasePath(String path);
		String getBasePath() const;

		void setDepth(int depth);

	private:
		time_t getFileWriteTime(String name) const;

		std::unique_ptr<ResourceLocator> locator;
		std::map<std::string, int> resourceTypeIds;
		std::vector<std::unique_ptr<ResourceCollectionBase>> resources;
		HalleyAPI* api;

		int curDepth = 0;
		String basePath;
	};
}
