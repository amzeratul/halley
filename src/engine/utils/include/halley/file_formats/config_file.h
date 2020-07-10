#pragma once

#include <map>
#include <vector>
#include "halley/text/halleystring.h"
#include "halley/maths/vector2.h"
#include "halley/resources/resource.h"
#include <gsl/span>

namespace Halley
{
	class ResourceLoader;
	class Serializer;
	class Deserializer;

	enum class ConfigNodeType
	{
		Undefined,
		String,
		Sequence,
		Map,
		Int,
		Float,
		Int2,
		Float2,
		Bytes
	};

	template <>
	struct EnumNames<ConfigNodeType> {
		constexpr std::array<const char*, 9> operator()() const {
			return{{
				"undefined",
				"string",
				"sequence",
				"map",
				"int",
				"float",
				"int2",
				"float2",
				"bytes"
			}};
		}
	};

	class ConfigFile;
	
	class ConfigNode
	{
		friend class ConfigFile;

	public:
		using MapType = std::map<String, ConfigNode>;
		using SequenceType = std::vector<ConfigNode>;

		ConfigNode();
		explicit ConfigNode(const ConfigNode& other);
		ConfigNode(ConfigNode&& other) noexcept;
		ConfigNode(MapType entryMap);
		ConfigNode(SequenceType entryList);
		ConfigNode(String value);
		ConfigNode(const char* value);
		ConfigNode(bool value);
		ConfigNode(int value);
		ConfigNode(float value);
		ConfigNode(Vector2i value);
		ConfigNode(Vector2f value);
		ConfigNode(Bytes value);

		template <typename T>
		ConfigNode(const std::vector<T>& sequence)
		{
			SequenceType seq;
			seq.reserve(sequence.size());
			for (auto& e: sequence) {
				seq.push_back(ConfigNode(e));
			}
			*this = seq;
		}

		~ConfigNode();
		
		ConfigNode& operator=(const ConfigNode& other) = delete;
		ConfigNode& operator=(ConfigNode&& other) noexcept;
		ConfigNode& operator=(bool value);
		ConfigNode& operator=(int value);
		ConfigNode& operator=(float value);
		ConfigNode& operator=(Vector2i value);
		ConfigNode& operator=(Vector2f value);

		ConfigNode& operator=(MapType entryMap);
		ConfigNode& operator=(SequenceType entryList);
		ConfigNode& operator=(String value);
		ConfigNode& operator=(Bytes value);
		ConfigNode& operator=(gsl::span<const gsl::byte> bytes);

		ConfigNode& operator=(const char* value);

		template <typename T>
		ConfigNode& operator=(const std::vector<T>& sequence)
		{
			SequenceType seq;
			seq.reserve(sequence.size());
			for (auto& e: sequence) {
				seq.push_back(ConfigNode(e));
			}
			return *this = seq;
		}

		ConfigNodeType getType() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		int asInt() const;
		float asFloat() const;
		bool asBool() const;
		Vector2i asVector2i() const;
		Vector2f asVector2f() const;
		String asString() const;
		const Bytes& asBytes() const;
		std::vector<String> asStringVector() const;

		int asInt(int defaultValue) const;
		float asFloat(float defaultValue) const;
		bool asBool(bool defaultValue) const;
		String asString(const String& defaultValue) const;
		Vector2i asVector2i(Vector2i defaultValue) const;
		Vector2f asVector2f(Vector2f defaultValue) const;
		std::vector<String> asStringVector(const std::vector<String>& defaultValue) const;

		const SequenceType& asSequence() const;
		const MapType& asMap() const;
		SequenceType& asSequence();
		MapType& asMap();

		void ensureType(ConfigNodeType type);

		bool hasKey(const String& key) const;
		void removeKey(const String& key);

		ConfigNode& operator[](const String& key);
		ConfigNode& operator[](size_t idx);

		const ConfigNode& operator[](const String& key) const;
		const ConfigNode& operator[](size_t idx) const;

		SequenceType::iterator begin();
		SequenceType::iterator end();

		SequenceType::const_iterator begin() const;
		SequenceType::const_iterator end() const;

		void reset();
		void setOriginalPosition(int line, int column);
		void setParent(const ConfigNode* parent, int idx);
		void propagateParentingInformation(const ConfigFile* parentFile);

		inline void assertValid() const
		{
			Expects(intData != 0xCDCDCDCD);
			Expects(intData != 0xDDDDDDDD);
		}

	private:
		union {
			void* ptrData;
			int intData;
			float floatData;
			Vector2i vec2iData;
			Vector2f vec2fData;
		};
		ConfigNodeType type = ConfigNodeType::Undefined;
		int line = 0;
		int column = 0;
		int parentIdx = 0;
		const ConfigNode* parent = nullptr;
		const ConfigFile* parentFile = nullptr;

		static ConfigNode undefinedConfigNode;
		static String undefinedConfigNodeName;

		template <typename T> void deserializeContents(Deserializer& s)
		{
			T v;
			s >> v;
			*this = std::move(v);
		}

		String getNodeDebugId() const;
		String backTrackFullNodeName() const;
	};

	class ConfigFile : public Resource
	{
	public:
		ConfigFile();
		explicit ConfigFile(const ConfigFile& other);
		ConfigFile(ConfigFile&& other) noexcept;

		ConfigFile& operator=(const ConfigFile& other);
		ConfigFile& operator=(ConfigFile&& other) noexcept;

		ConfigNode& getRoot();
		const ConfigNode& getRoot() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		static std::unique_ptr<ConfigFile> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::ConfigFile; }

		void reload(Resource&& resource) override;

	protected:
		ConfigNode root;
		bool storeFilePosition = true;

		void updateRoot();
	};

	class ConfigObserver
	{
	public:
		ConfigObserver();
		ConfigObserver(const ConfigNode& node);
		ConfigObserver(const ConfigFile& file);

		const ConfigNode& getRoot() const;
		
		bool needsUpdate() const;
		void update();
		String getAssetId() const;

	private:
		int assetVersion = 0;
		const ConfigFile* file = nullptr;
		const ConfigNode* node = nullptr;
	};
	
	class Prefab : public ConfigFile {
	public:
		static std::unique_ptr<Prefab> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Prefab; }

		void reload(Resource&& resource) override;
		void makeDefault();
	};

	class Scene final : public Prefab {
	public:
		static std::unique_ptr<Scene> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Scene; }

		void reload(Resource&& resource) override;
		void makeDefault();
	};
}
