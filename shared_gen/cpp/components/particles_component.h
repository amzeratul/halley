// Halley codegen version 125
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif
#include "halley/support/exception.h"


class ParticlesComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 7 };
	static const constexpr char* componentName{ "Particles" };

	Halley::Particles particles{};
	Halley::Vector<Halley::Sprite> sprites{};
	Halley::ResourceReference<Halley::Animation> animation{};
	int layer{ 0 };
	Halley::OptionalLite<int> mask{};

	ParticlesComponent() {
	}

	ParticlesComponent(Halley::Particles particles, Halley::Vector<Halley::Sprite> sprites, Halley::ResourceReference<Halley::Animation> animation, int layer, Halley::OptionalLite<int> mask)
		: particles(std::move(particles))
		, sprites(std::move(sprites))
		, animation(std::move(animation))
		, layer(std::move(layer))
		, mask(std::move(mask))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(particles)>::serialize(particles, Halley::Particles{}, _context, _node, componentName, "particles", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sprites)>::serialize(sprites, Halley::Vector<Halley::Sprite>{}, _context, _node, componentName, "sprites", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(animation)>::serialize(animation, Halley::ResourceReference<Halley::Animation>{}, _context, _node, componentName, "animation", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, int{ 0 }, _context, _node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, Halley::OptionalLite<int>{}, _context, _node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(particles)>::deserialize(particles, Halley::Particles{}, _context, _node, componentName, "particles", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sprites)>::deserialize(sprites, Halley::Vector<Halley::Sprite>{}, _context, _node, componentName, "sprites", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(animation)>::deserialize(animation, Halley::ResourceReference<Halley::Animation>{}, _context, _node, componentName, "animation", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, int{ 0 }, _context, _node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, Halley::OptionalLite<int>{}, _context, _node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "particles") {
			return Halley::ConfigNodeHelper<decltype(particles)>::serialize(particles, _context);
		}
		if (_fieldName == "sprites") {
			return Halley::ConfigNodeHelper<decltype(sprites)>::serialize(sprites, _context);
		}
		if (_fieldName == "animation") {
			return Halley::ConfigNodeHelper<decltype(animation)>::serialize(animation, _context);
		}
		if (_fieldName == "layer") {
			return Halley::ConfigNodeHelper<decltype(layer)>::serialize(layer, _context);
		}
		if (_fieldName == "mask") {
			return Halley::ConfigNodeHelper<decltype(mask)>::serialize(mask, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "particles") {
			Halley::ConfigNodeHelper<decltype(particles)>::deserialize(particles, _context, _node);
			return;
		}
		if (_fieldName == "sprites") {
			Halley::ConfigNodeHelper<decltype(sprites)>::deserialize(sprites, _context, _node);
			return;
		}
		if (_fieldName == "animation") {
			Halley::ConfigNodeHelper<decltype(animation)>::deserialize(animation, _context, _node);
			return;
		}
		if (_fieldName == "layer") {
			Halley::ConfigNodeHelper<decltype(layer)>::deserialize(layer, _context, _node);
			return;
		}
		if (_fieldName == "mask") {
			Halley::ConfigNodeHelper<decltype(mask)>::deserialize(mask, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

};
