#include "halley/file_formats/json_convert.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/text/encode.h"
#include "json/json.h"
using namespace Halley;

namespace {
	Json::Value configToJson(const ConfigNode& src)
	{
		Json::Value result;

		switch (src.getType()) {
		case ConfigNodeType::Sequence:
		case ConfigNodeType::DeltaSequence:
			result = Json::Value(Json::arrayValue);
			for (const auto& n: src.asSequence()) {
				result.append(configToJson(n));
			}
			break;

		case ConfigNodeType::Map:
		case ConfigNodeType::DeltaMap:
			result = Json::Value(Json::objectValue);
			for (const auto& [k, v]: src.asMap()) {
				result[k.cppStr()] = configToJson(v);
			}
			break;

		case ConfigNodeType::String:
			result = src.asString().cppStr();
			break;
		case ConfigNodeType::Bytes:
			result = Encode::encodeBase64(src.asBytes().const_byte_span()).cppStr();
			break;

		case ConfigNodeType::Int:
			result = src.asInt();
			break;
		case ConfigNodeType::Int64:
		case ConfigNodeType::EntityId:
			result = static_cast<Json::Int64>(src.asInt64());
			break;
		case ConfigNodeType::Float:
			result = src.asFloat();
			break;
		case ConfigNodeType::Bool:
			result = src.asBool();
			break;

		case ConfigNodeType::Int2:
			{
				const auto v = src.asVector2i();
				result = Json::Value(Json::arrayValue);
				result.append(Json::Value(v.x));
				result.append(Json::Value(v.y));
			}
			break;
		case ConfigNodeType::Float2:
			{
				const auto v = src.asVector2f();
				result = Json::Value(Json::arrayValue);
				result.append(Json::Value(v.x));
				result.append(Json::Value(v.y));
			}
			break;

		case ConfigNodeType::Noop:
		case ConfigNodeType::Idx:
		case ConfigNodeType::Del:
		case ConfigNodeType::Undefined:
		default:
			break;
		}

		return result;
	}

	ConfigNode jsonToConfig(const Json::Value& src)
	{
		ConfigNode result;

		switch (src.type()) {
		case Json::arrayValue:
			{
				auto seq = ConfigNode::SequenceType();
				auto n = src.size();
				for (decltype(n) i = 0 ; i < n; ++i) {
					seq.push_back(jsonToConfig(src[i]));
				}
				result = std::move(seq);
			}
			break;
		case Json::objectValue:
			result = ConfigNode::MapType();
			for (const auto& k: src.getMemberNames()) {
				result[k] = jsonToConfig(src[k]);
			}
			break;

		case Json::intValue:
			result = src.asInt();
			break;
		case Json::uintValue:
			result = src.asUInt();
			break;
		case Json::realValue:
			result = src.asFloat();
			break;
		case Json::stringValue:
			result = String(src.asString());
			break;
		case Json::booleanValue:
			result = src.asBool();
			break;
		case Json::nullValue:
		default:
			break;
		}

		return result;
	}
}

String JSONConvert::generateJSON(const ConfigNode& config, const Options& options)
{
	const Json::Value rootNode = configToJson(config);

	if (options.compact) {
		Json::FastWriter writer;
		return writer.write(rootNode);
	} else {
		Json::StyledStreamWriter writer;
		std::ostringstream str;
		writer.write(str, rootNode);
		return str.str();
	}
}

ConfigNode JSONConvert::parseConfig(gsl::span<const std::byte> data)
{
	Json::Value root;
	Json::Reader reader;
	const char* src = reinterpret_cast<const char*>(data.data());
	if (reader.parse(src, src + data.size(), root)) {
		return jsonToConfig(root);
	} else {
		Logger::logError("Failed to parse JSON:\n" + String(src, data.size()));
		return {};
	}
}

ConfigNode JSONConvert::parseConfig(const Bytes& data)
{
	return parseConfig(data.const_byte_span());
}

ConfigNode JSONConvert::parseConfig(const String& str)
{
	return parseConfig(gsl::as_bytes(str.asSpan()));
}
