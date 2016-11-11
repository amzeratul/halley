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
				, depth(other.depth)
			{}

			Wrapper(std::shared_ptr<Resource> resource, int loadDepth)
				: res(resource)
				, depth(loadDepth)
			{}

			void flush();
			std::shared_ptr<Resource> res;
			int depth;
		};

	public:
		explicit ResourceCollectionBase(Resources& parent, const String& path);
		virtual ~ResourceCollectionBase() {}

		void setResource(int curDepth, const String& name, std::shared_ptr<Resource> resource);
		void clear();
		void unload(const String& name);
		void unloadAll(int minDepth = 0);
		void flush(const String& name);
		
		String getPath() const { return path; }
		String resolveName(const String& name) const;

	protected:
		virtual std::shared_ptr<Resource> loadResource(ResourceLoader& loader) = 0;

		std::shared_ptr<Resource> doGet(const String& name, ResourceLoadPriority priority);

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
		ResourceCollection(Resources& parent, const String& path)
			: ResourceCollectionBase(parent, path)
		{}

		std::shared_ptr<const T> get(const String& name, ResourceLoadPriority priority = ResourceLoadPriority::Normal)
		{
			return std::static_pointer_cast<T>(doGet(name, priority));
		}

	protected:
		std::shared_ptr<Resource> loadResource(ResourceLoader& loader) override {
			return T::loadResource(loader);
		}
	};
}
