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
	
	namespace ResourceLoadPriority {
		enum Type {
			Low = 0,
			Normal = 1,
			High = 2
		};
	}

	template <typename T>
	class ResourceWrapper {
	public:
		ResourceWrapper() {}

		ResourceWrapper(std::shared_ptr<T> resource, int loadDepth, std::time_t time)
			: res(resource)
			, lastWriteTime(time)
			, depth(loadDepth)
		{}

		std::shared_ptr<T> res;
		std::time_t lastWriteTime;
		int depth;
	};

	/*
	std::time_t getFileWriteTime(String name);
	
	template <typename T>
	class ResourceManager {
	public:
		static std::shared_ptr<T> get(String name, ResourceLoadPriority::Type priority = ResourceLoadPriority::Normal)
		{
			return getInstance().doGet(name, priority);
		}

		static void preLoad(String resource)
		{
			getInstance().doGet(resource, ResourceLoadPriority::Low);
		}

		static void setResource(String _name, std::shared_ptr<T> resource)
		{
			String name = getInstance().resolveName(_name);
			getInstance().resources[name] = ResourceWrapper<T>(resource, getInstance().curDepth);
		}

		static void setBasePath(String path)
		{
			getInstance().basePath = path;
		}

		static void init()
		{
			setLoader(T::loadResource);
		}

		static void clear()
		{
			getInstance().resources.clear();
		}

		static void unload(String name)
		{
			getInstance().resources.erase(name);
		}

		static void unloadAll(int minDepth=0)
		{
			auto& resources = getInstance().resources;
			for (auto iter = resources.begin(); iter != resources.end(); ) {
				auto next = iter;
				next++;

				auto& res = (*iter).second;
				if (res.depth >= minDepth) {
					//std::cout << "Unloaded " << (*iter).first << "\n";
					resources.erase(iter);
				}

				iter = next;
			}
		}

		static void flush(String name)
		{
			auto res = getInstance().resources.find(name);
			if (res != getInstance().resources.end()) {
				auto& resWrap = res.second;
				resWrap.res->flush();
			}
		}

		static void flushAll(int minDepth=0)
		{
			auto& resources = getInstance().resources;
			for (auto i = resources.begin(); i != resources.end(); i++) {
				auto& res = (*i).second;
				String name = (*i).first;
				auto curTime = getFileWriteTime(name);
				if (res.depth >= minDepth && res.lastWriteTime != curTime) {
					std::cout << "Flushing \"" << ConsoleColor(Console::DARK_GREY) << name << ConsoleColor() << "\"...\n";
					res.res->flush();
					res.lastWriteTime = curTime;
				}
			}
		}

		static void setLoader(std::function<shared_ptr<T>(String, ResourceLoadPriority::Type)> f)
		{
			getInstance().loader = f;
		}

		static String getName(String name)
		{
			return getInstance().resolveName(name);
		}

		static String getBasePath()
		{
			return getInstance().basePath;
		}

		static void setDepth(int depth)
		{
			getInstance().curDepth = depth;
		}

	private:
		ResourceManager()
			: curDepth(0)
		{
		}

		static ResourceManager<T>& getInstance()
		{
			if (!instance) instance = shared_ptr<ResourceManager<T> >(new ResourceManager<T>());
			return *instance;
		}

		std::shared_ptr<T> load(String name, ResourceLoadPriority::Type priority)
		{
			return loader(name, priority);
		}
		
		Halley::String resolveName(String name)
		{
			return basePath + name;
		}
		
		std::shared_ptr<T> doGet(String _name, ResourceLoadPriority::Type priority)
		{
			String name = resolveName(_name);

			// Look in cache
			auto res = resources.find(name);
			if (res != resources.end()) return res->second.res;

			// Not found, load it from disk
			auto newRes = load(name, priority);
			if (!newRes) throw Exception("Unable to find resource: "+name);

			// Store in cache
			std::time_t time = getFileWriteTime(name);
			resources[name] = ResourceWrapper<T>(newRes, curDepth, time);

			return newRes;
		}

		int curDepth;
		std::map<Halley::String, ResourceWrapper<T> > resources;
		Halley::String basePath;
		std::function<shared_ptr<T>(String, ResourceLoadPriority::Type)> loader;

		static shared_ptr<ResourceManager<T> > instance;
	};

	template<class T> shared_ptr<ResourceManager<T> > ResourceManager<T>::instance;
	*/
}
