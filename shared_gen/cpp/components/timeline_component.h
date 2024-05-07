// Halley codegen version 128
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class TimelineComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 16 };
	static const constexpr char* componentName{ "Timeline" };

	Halley::Timeline timeline{};
	Halley::TimelinePlayer player{};
	bool playOnStart{ false };

	TimelineComponent() {
	}

	TimelineComponent(Halley::Timeline timeline, bool playOnStart)
		: timeline(std::move(timeline))
		, playOnStart(std::move(playOnStart))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(timeline)>::serialize(timeline, Halley::Timeline{}, _context, _node, componentName, "timeline", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(player)>::serialize(player, Halley::TimelinePlayer{}, _context, _node, componentName, "player", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(playOnStart)>::serialize(playOnStart, bool{ false }, _context, _node, componentName, "playOnStart", makeMask(Type::Prefab));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(timeline)>::deserialize(timeline, Halley::Timeline{}, _context, _node, componentName, "timeline", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(player)>::deserialize(player, Halley::TimelinePlayer{}, _context, _node, componentName, "player", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(playOnStart)>::deserialize(playOnStart, bool{ false }, _context, _node, componentName, "playOnStart", makeMask(Type::Prefab));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "player") {
			return Halley::ConfigNodeHelper<decltype(player)>::serialize(player, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "player") {
			Halley::ConfigNodeHelper<decltype(player)>::deserialize(player, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<TimelineComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<TimelineComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<TimelineComponent>(ptr);
	}

};
