#pragma once

#include <gsl/span>

#include "halley/text/halleystring.h"
#include "halley/resources/resource.h"
#include "halley/maths/range.h"
#include "halley/maths/vector4.h"
#include <map>
#include <vector>

namespace Halley {
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
		Bytes,
		Noop, // For delta coding
		Idx, // For delta coding
		Del // For delta coding
	};

	template <>
	struct EnumNames<ConfigNodeType> {
		constexpr std::array<const char*, 11> operator()() const {
			return{{
				"undefined",
				"string",
				"sequence",
				"map",
				"int",
				"float",
				"int2",
				"float2",
				"bytes",
				"noop",
				"idx"
				"del"
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
		explicit ConfigNode(String value);
		explicit ConfigNode(const char* value);
		explicit ConfigNode(bool value);
		explicit ConfigNode(int value);
		explicit ConfigNode(float value);
		explicit ConfigNode(Vector2i value);
		explicit ConfigNode(Vector2f value);
		explicit ConfigNode(Bytes value);

		template <typename T>
		explicit ConfigNode(const std::vector<T>& sequence)
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

		bool operator==(const ConfigNode& other) const;
		bool operator!=(const ConfigNode& other) const;

		ConfigNodeType getType() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		int asInt() const;
		float asFloat() const;
		bool asBool() const;
		Vector2i asVector2i() const;
		Vector2f asVector2f() const;
		Vector4i asVector4i() const;
		Vector4f asVector4f() const;
		Range<float> asFloatRange() const;
		String asString() const;
		const Bytes& asBytes() const;

		int asInt(int defaultValue) const;
		float asFloat(float defaultValue) const;
		bool asBool(bool defaultValue) const;
		String asString(const String& defaultValue) const;
		Vector2i asVector2i(Vector2i defaultValue) const;
		Vector2f asVector2f(Vector2f defaultValue) const;

		template <typename T>
		std::vector<T> asVector() const
		{
			if (type == ConfigNodeType::Sequence) {
				std::vector<T> result;
				result.reserve(asSequence().size());
				for (const auto& e : asSequence()) {
					result.emplace_back(e.convertTo(Tag<T>()));
				}
				return result;
			} else {
				throw Exception("Can't convert " + getNodeDebugId() + " from " + toString(getType()) + " to std::vector<T>.", HalleyExceptions::Resources);
			}
		}

		template <typename T>
		std::vector<T> asVector(const std::vector<T>& defaultValue) const
		{
			if (type == ConfigNodeType::Sequence) {
				return asVector<T>();
			} else {
				return defaultValue;
			}
		}
		
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

		static ConfigNode createDelta(const ConfigNode& from, const ConfigNode& to);
		void applyDelta(const ConfigNode& delta);

	private:
		template <typename T>
		class Tag {};

		struct NoopType {};
		struct DelType {};
		struct IdxType {
			int start;
			int len;
			IdxType() = default;
			IdxType(int start, int len) : start(start), len(len) {}
		};
		
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

		explicit ConfigNode(NoopType value);
		explicit ConfigNode(DelType value);
		explicit ConfigNode(IdxType value);
		ConfigNode& operator=(NoopType value);
		ConfigNode& operator=(DelType value);
		ConfigNode& operator=(IdxType value);

		String getNodeDebugId() const;
		String backTrackFullNodeName() const;

		int convertTo(Tag<int> tag) const;
		float convertTo(Tag<float> tag) const;
		bool convertTo(Tag<bool> tag) const;
		Vector2i convertTo(Tag<Vector2i> tag) const;
		Vector2f convertTo(Tag<Vector2f> tag) const;
		Vector4i convertTo(Tag<Vector4i> tag) const;
		Vector4f convertTo(Tag<Vector4f> tag) const;
		Range<float> convertTo(Tag<Range<float>> tag) const;
		String convertTo(Tag<String> tag) const;
		const Bytes& convertTo(Tag<Bytes&> tag) const;

		static ConfigNode createMapDelta(const ConfigNode& from, const ConfigNode& to);
		static ConfigNode createSequenceDelta(const ConfigNode& from, const ConfigNode& to);
	};
}
