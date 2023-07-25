// Halley codegen version 123
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

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
		Halley::EntityConfigNodeSerializer<decltype(particles)>::serialize(particles, Halley::Particles{}, _context, _node, componentName, "particles", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sprites)>::serialize(sprites, Halley::Vector<Halley::Sprite>{}, _context, _node, componentName, "sprites", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(animation)>::serialize(animation, Halley::ResourceReference<Halley::Animation>{}, _context, _node, componentName, "animation", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, int{ 0 }, _context, _node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, Halley::OptionalLite<int>{}, _context, _node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(particles)>::deserialize(particles, Halley::Particles{}, _context, _node, componentName, "particles", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sprites)>::deserialize(sprites, Halley::Vector<Halley::Sprite>{}, _context, _node, componentName, "sprites", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(animation)>::deserialize(animation, Halley::ResourceReference<Halley::Animation>{}, _context, _node, componentName, "animation", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, int{ 0 }, _context, _node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, Halley::OptionalLite<int>{}, _context, _node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
