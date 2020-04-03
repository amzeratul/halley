#include "halley/tools/yaml/yaml_convert.h"
#include "halley/bytes/byte_serializer.h"
#include "halley-yamlcpp.h"
#include "halley/tools/file/filesystem.h"
using namespace Halley;

ConfigNode YAMLConvert::parseYAMLNode(const YAML::Node& node)
{
	ConfigNode result;

	if (node.IsMap()) {
		std::map<String, ConfigNode> map;
		for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
			String key = it->first.as<std::string>();
			map[key] = parseYAMLNode(it->second);
		}
		result = std::move(map);
	}
	else if (node.IsSequence()) {
		std::vector<ConfigNode> list;
		for (auto& n : node) {
			list.emplace_back(parseYAMLNode(n));
		}
		result = std::move(list);
	}
	else if (node.IsScalar()) {
		auto str = String(node.as<std::string>());
		if (str.isNumber()) {
			if (str.isInteger()) {
				result = str.toInteger();
			} else {
				result = str.toFloat();
			}
		}
		else {
			result = str;
		}
	}

	result.setOriginalPosition(node.Mark().line, node.Mark().column);
	return result;
}

void YAMLConvert::parseConfig(ConfigFile& config, gsl::span<const gsl::byte> data)
{
	String strData(reinterpret_cast<const char*>(data.data()), data.size());
	YAML::Node root = YAML::Load(strData.cppStr());

	config.getRoot() = parseYAMLNode(root);
}

ConfigFile YAMLConvert::parseConfig(gsl::span<const gsl::byte> data)
{
	ConfigFile config;
	parseConfig(config, data);
	return config;
}

ConfigFile YAMLConvert::parseConfig(const Bytes& data)
{
	return parseConfig(gsl::as_bytes(gsl::span<const Byte>(data.data(), data.size())));
}

String YAMLConvert::generateYAML(const ConfigFile& config, const EmitOptions& options)
{
	return generateYAML(config.getRoot(), options);
}

String YAMLConvert::generateYAML(const ConfigNode& node, const EmitOptions& options)
{
	YAML::Emitter emitter;
	emitNode(node, emitter, options);

	if (!emitter.good()) {
		throw Exception("Error generating YAML: " + emitter.GetLastError(), HalleyExceptions::Tools);
	}
	return emitter.c_str();
}

void YAMLConvert::emitNode(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options)
{
	switch (node.getType()) {
	case ConfigNodeType::Int:
		emitter << node.asInt();
		return;

	case ConfigNodeType::Int2:
		{
			const auto vec = node.asVector2i();
			emitter << YAML::Flow << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
		}
		return;

	case ConfigNodeType::Float:
		emitter << node.asFloat();
		return;

	case ConfigNodeType::Float2:
		{
			const auto vec = node.asVector2f();
			emitter << YAML::Flow << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
		}
		return;

	case ConfigNodeType::Sequence:
		emitSequence(node, emitter, options);
		return;

	case ConfigNodeType::Map:
		emitMap(node, emitter, options);
		return;

	case ConfigNodeType::String:
		emitter << node.asString().cppStr();
		return;

	case ConfigNodeType::Undefined:
		emitter << YAML::Null;
		return;
		
	case ConfigNodeType::Bytes:
		throw Exception("Unsupported ConfigNode type: bytes", HalleyExceptions::Tools);
	}
}

void YAMLConvert::emitSequence(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options)
{
	if (isCompactSequence(node, 0)) {
		emitter << YAML::Flow;
	}

	emitter << YAML::BeginSeq;
	for (auto& n: node.asSequence()) {
		emitNode(n, emitter, options);
	}
	emitter << YAML::EndSeq;
}

void YAMLConvert::emitMap(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options)
{
	const auto& map = node.asMap();

	if (map.empty()) {
		emitter << YAML::Flow;
	}
	emitter << YAML::BeginMap;

	// Sort entries by key, using options
	std::vector<String> keys;
	keys.reserve(map.size());
	for (auto& kv: map) {
		keys.push_back(kv.first);
	}

	std::sort(keys.begin(), keys.end(), [&] (const String& a, const String& b)
	{
		const auto& mko = options.mapKeyOrder;
		const auto idxA = std::find(mko.begin(), mko.end(), a);
		const auto idxB = std::find(mko.begin(), mko.end(), b);
		if (idxA != idxB) {
			return idxA < idxB;
		}
		return a < b;
	});

	// Emit them in order
	for (const auto& k: keys) {
		const auto iter = map.find(k);
		emitter << YAML::Key << k;
		emitter << YAML::Value;
		emitNode(iter->second, emitter, options);
	}
	
	emitter << YAML::EndMap;
}

bool YAMLConvert::isCompactSequence(const ConfigNode& node, int depth)
{	
	switch (node.getType()) {
	case ConfigNodeType::Map:
		return false;

	case ConfigNodeType::Int:
	case ConfigNodeType::Float:
	case ConfigNodeType::String:
		return true;

	case ConfigNodeType::Int2:
	case ConfigNodeType::Float2:
		return depth <= 2;

	case ConfigNodeType::Sequence:
		if (depth >= 2) {
			return false;
		}
		bool ok = true;
		for (auto& n: node.asSequence()) {
			ok = ok && isCompactSequence(n, depth + 1);
		}
		return ok;
	}

	return false;
}
