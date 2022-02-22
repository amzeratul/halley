// Halley codegen version 102
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ParticlesComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 5 };
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

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(particles)>::serialize(particles, Halley::Particles{}, context, node, componentName, "particles", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sprites)>::serialize(sprites, Halley::Vector<Halley::Sprite>{}, context, node, componentName, "sprites", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(animation)>::serialize(animation, Halley::ResourceReference<Halley::Animation>{}, context, node, componentName, "animation", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, int{ 0 }, context, node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, Halley::OptionalLite<int>{}, context, node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(particles)>::deserialize(particles, Halley::Particles{}, context, node, componentName, "particles", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sprites)>::deserialize(sprites, Halley::Vector<Halley::Sprite>{}, context, node, componentName, "sprites", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(animation)>::deserialize(animation, Halley::ResourceReference<Halley::Animation>{}, context, node, componentName, "animation", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, int{ 0 }, context, node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, Halley::OptionalLite<int>{}, context, node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
