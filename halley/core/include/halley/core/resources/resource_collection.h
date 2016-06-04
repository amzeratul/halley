#pragma once

#include <utility>
#include <memory>
#include <halley/text/halleystring.h>
#include <halley/resources/resource_data.h>
#include <halley/data_structures/hash_map.h>

namespace Halley
{
	class Resource;
	class Resources;
	class ResourceLoader;

	class ResourceCollectionBase
	{
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
		explicit ResourceCollectionBase(Resources& parent, String path);
		virtual ~ResourceCollectionBase() {}

		void setResource(int curDepth, String name, std::shared_ptr<Resource> resource);
		void clear();
		void unload(String name);
		void unloadAll(int minDepth = 0);
		void flush(String name);
		void flushAll(int minDepth = 0);
		
		String getPath() const { return path; }
		String resolveName(String name) const;

	protected:
		virtual std::unique_ptr<Resource> loadResource(ResourceLoader& loader) = 0;

		std::shared_ptr<Resource> doGet(String name, ResourceLoadPriority priority);

	private:
		Resources& parent;
		String path;
		HashMap<String, Wrapper> resources;
	};

	template <typename T>
	class ResourceCollection : public ResourceCollectionBase
	{
		static_assert(std::is_base_of<Resource, T>::value, "Type must extend Resource");

	public:
		ResourceCollection(Resources& parent, String path)
			: ResourceCollectionBase(parent, path)
		{}

		std::shared_ptr<T> get(String name, ResourceLoadPriority priority = ResourceLoadPriority::Normal)
		{
			return std::static_pointer_cast<T>(doGet(name, priority));
		}

	protected:
		std::unique_ptr<Resource> loadResource(ResourceLoader& loader) override {
			return T::loadResource(loader);
		}
	};
}
