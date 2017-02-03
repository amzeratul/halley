#pragma once

#include <utility>
#include <memory>
#include <halley/text/halleystring.h>
#include <halley/resources/resource_data.h>
#include <halley/data_structures/hash_map.h>

namespace Halley
{
	enum class AssetType;
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
		explicit ResourceCollectionBase(Resources& parent, AssetType type);
		virtual ~ResourceCollectionBase() {}

		void setResource(int curDepth, const String& assetId, std::shared_ptr<Resource> resource);
		void clear();
		void unload(const String& assetId);
		void unloadAll(int minDepth = 0);
		void flush(const String& assetId);
		std::vector<String> enumerate() const;

	protected:
		virtual std::shared_ptr<Resource> loadResource(ResourceLoader& loader) = 0;

		std::shared_ptr<Resource> doGet(const String& name, ResourceLoadPriority priority);

	private:
		Resources& parent;
		HashMap<String, Wrapper> resources;
		AssetType type;
	};

	template <typename T>
	class ResourceCollection : public ResourceCollectionBase
	{
		static_assert(std::is_base_of<Resource, T>::value, "Type must extend Resource");

	public:
		ResourceCollection(Resources& parent, AssetType type)
			: ResourceCollectionBase(parent, type)
		{}

		std::shared_ptr<const T> get(const String& assetId, ResourceLoadPriority priority = ResourceLoadPriority::Normal)
		{
			return std::static_pointer_cast<T>(doGet(assetId, priority));
		}

	protected:
		std::shared_ptr<Resource> loadResource(ResourceLoader& loader) override {
			return T::loadResource(loader);
		}
	};
}
