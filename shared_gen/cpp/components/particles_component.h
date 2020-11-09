#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ParticlesComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 5 };
	static const constexpr char* componentName{ "Particles" };

	Halley::Particles particles{};
	std::vector<Halley::Sprite> sprites{};
	Halley::ResourceReference<Halley::Animation> animation{};
	int layer{ 0 };
	Halley::OptionalLite<int> mask{};

	ParticlesComponent() {
	}

	ParticlesComponent(Halley::Particles particles, std::vector<Halley::Sprite> sprites, Halley::ResourceReference<Halley::Animation> animation, int layer, Halley::OptionalLite<int> mask)
		: particles(std::move(particles))
		, sprites(std::move(sprites))
		, animation(std::move(animation))
		, layer(std::move(layer))
		, mask(std::move(mask))
	{
	}

	Halley::ConfigNode serialize(const Halley::ConfigNodeSerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(particles)>::serialize(particles, Halley::Particles{}, context, node, "particles", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(sprites)>::serialize(sprites, std::vector<Halley::Sprite>{}, context, node, "sprites", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(animation)>::serialize(animation, Halley::ResourceReference<Halley::Animation>{}, context, node, "animation", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, int{ 0 }, context, node, "layer", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, Halley::OptionalLite<int>{}, context, node, "mask", makeMask(Type::Prefab, Type::SaveData));
		return node;
	}

	void deserialize(const Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(particles)>::deserialize(particles, context, node, "particles", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(sprites)>::deserialize(sprites, context, node, "sprites", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(animation)>::deserialize(animation, context, node, "animation", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, context, node, "layer", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, context, node, "mask", makeMask(Type::Prefab, Type::SaveData));
	}

};
