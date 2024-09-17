// Halley codegen version 136
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/entity/component.h"
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
	Halley::OptionalLite<Halley::SpriteMaskBase> mask{};

	ParticlesComponent() {
	}

	ParticlesComponent(Halley::Particles particles, Halley::Vector<Halley::Sprite> sprites, Halley::ResourceReference<Halley::Animation> animation, int layer, Halley::OptionalLite<Halley::SpriteMaskBase> mask)
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
		Halley::EntityConfigNodeSerializer<decltype(particles)>::serialize(particles, Halley::Particles{}, _context, _node, componentName, "particles", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(sprites)>::serialize(sprites, Halley::Vector<Halley::Sprite>{}, _context, _node, componentName, "sprites", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(animation)>::serialize(animation, Halley::ResourceReference<Halley::Animation>{}, _context, _node, componentName, "animation", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, int{ 0 }, _context, _node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, Halley::OptionalLite<Halley::SpriteMaskBase>{}, _context, _node, componentName, "mask", makeMask(Type::Prefab));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(particles)>::deserialize(particles, Halley::Particles{}, _context, _node, componentName, "particles", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(sprites)>::deserialize(sprites, Halley::Vector<Halley::Sprite>{}, _context, _node, componentName, "sprites", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(animation)>::deserialize(animation, Halley::ResourceReference<Halley::Animation>{}, _context, _node, componentName, "animation", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, int{ 0 }, _context, _node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, Halley::OptionalLite<Halley::SpriteMaskBase>{}, _context, _node, componentName, "mask", makeMask(Type::Prefab));
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("particles");
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("sprites");
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("animation");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("layer");
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("mask");
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "layer") {
			return Halley::ConfigNodeHelper<decltype(layer)>::serialize(layer, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "layer") {
			Halley::ConfigNodeHelper<decltype(layer)>::deserialize(layer, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<ParticlesComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<ParticlesComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<ParticlesComponent>(ptr);
	}

};
