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

namespace Halley {
	
	enum class ResourceLoadPriority {
		Low = 0,
		Normal = 1,
		High = 2
	};

	class ResourceLocator;
	class HalleyAPI;

	class ResourceLoader
	{
		friend class Resources;

	public:
		String getName() const { return name; }
		ResourceLoadPriority getPriority() const { return priority; }
		std::unique_ptr<ResourceDataStatic> getStatic();
		std::unique_ptr<ResourceDataStream> getStream();
		HalleyAPI& getAPI() const { return *api; }

	private:
		ResourceLoader(ResourceLoader&& loader);
		ResourceLoader(ResourceLocator& locator, String name, ResourceLoadPriority priority, HalleyAPI* api);

		ResourceLocator& locator;
		String name;
		ResourceLoadPriority priority;
		HalleyAPI* api;
		bool loaded = false;
	};

	class Resources {
		class Wrapper
		{
		public:
			Wrapper(Wrapper&& other)
				: res(std::move(other.res))
				, lastWriteTime(other.lastWriteTime)
				, depth(other.depth)
			{}

			Wrapper(std::shared_ptr<Resource> resource, int loadDepth, time_t time)
				: res(resource)
				, lastWriteTime(time)
				, depth(loadDepth)
			{}

			void flush();
			std::shared_ptr<Resource> res;
			time_t lastWriteTime;
			int depth;
		};

	public:
		Resources(std::unique_ptr<ResourceLocator> locator, HalleyAPI* api);
		~Resources();

		template <typename T>
		std::shared_ptr<T> get(String name, ResourceLoadPriority priority = ResourceLoadPriority::Normal) { return std::static_pointer_cast<T>(doGet(name, priority, &Resources::loader<T>)); }

		template <typename T>
		void preLoad(String name) {	doGet(name, ResourceLoadPriority::Low, loader<T>); }
		
		void setResource(String _name, std::shared_ptr<Resource> resource);

		String getFullName(String name) const;
		void setBasePath(String path);
		String getBasePath() const;

		void clear();
		void unload(String name);
		void unloadAll(int minDepth = 0);
		void flush(String name);
		void flushAll(int minDepth = 0);
		
		void setDepth(int depth);

	private:
		String resolveName(String name) const;
		std::shared_ptr<Resource> doGet(String _name, ResourceLoadPriority priority, std::function<std::unique_ptr<Resource>(ResourceLoader&)> loader);
		time_t getFileWriteTime(String name);

		template <typename T>
		static std::unique_ptr<Resource> loader(ResourceLoader& loader)
		{
			return T::loadResource(loader);
		}

		std::unique_ptr<ResourceLocator> locator;
		std::map<String, Wrapper> resources;
		HalleyAPI* api;

		int curDepth = 0;
		String basePath;
	};
}
