#pragma once

#include <utility>
#include <memory>
#include <functional>
#include <shared_mutex>
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
			Wrapper(Wrapper&& other) noexcept
				: res(std::move(other.res))
				, depth(other.depth)
			{}

			Wrapper(std::shared_ptr<Resource> resource, int loadDepth)
				: res(std::move(resource))
				, depth(loadDepth)
			{}

			std::shared_ptr<Resource> res;
			int depth;
		};

	public:
		using ResourceLoaderFunc = std::function<std::shared_ptr<Resource>(const String&, ResourceLoadPriority)>;
		using ResourceEnumeratorFunc = std::function<std::vector<String>()>;

		explicit ResourceCollectionBase(Resources& parent, AssetType type);
		virtual ~ResourceCollectionBase() {}

		void setResource(int curDepth, const String& assetId, std::shared_ptr<Resource> resource);
		void setResourceLoader(ResourceLoaderFunc loader);
		void setResourceEnumerator(ResourceEnumeratorFunc enumerator);

		void clear();
		void unload(const String& assetId);
		void unloadAll(int minDepth = 0);
		bool exists(const String& assetId) const;
		void setFallback(const String& assetId);

		void reload(const String& assetId);
		void purge(const String& assetId);

		std::shared_ptr<Resource> getUntyped(const String& name, ResourceLoadPriority priority = ResourceLoadPriority::Normal);

		std::vector<String> enumerate() const;

	protected:
		virtual std::shared_ptr<Resource> loadResource(ResourceLoader& loader) = 0;

		std::shared_ptr<Resource> doGet(const String& name, ResourceLoadPriority priority, bool allowFallback);
		std::pair<std::shared_ptr<Resource>, bool> loadAsset(const String& assetId, ResourceLoadPriority priority, bool allowFallback);

	private:
		Resources& parent;
		HashMap<String, Wrapper> resources;
		String fallback;
		AssetType type;
		ResourceLoaderFunc resourceLoader;
		ResourceEnumeratorFunc resourceEnumerator;
		std::shared_mutex mutex;
	};

	template <typename T>
	class ResourceCollection final : public ResourceCollectionBase
	{
		static_assert(std::is_base_of<Resource, T>::value, "Type must extend Resource");

	public:
		ResourceCollection(Resources& parent, AssetType type)
			: ResourceCollectionBase(parent, type)
		{}

		std::shared_ptr<const T> get(const String& assetId, ResourceLoadPriority priority = ResourceLoadPriority::Normal)
		{
			return std::static_pointer_cast<T>(doGet(assetId, priority, true));
		}

	protected:
		std::shared_ptr<Resource> loadResource(ResourceLoader& loader) override {
			return T::loadResource(loader);
		}
	};
}
