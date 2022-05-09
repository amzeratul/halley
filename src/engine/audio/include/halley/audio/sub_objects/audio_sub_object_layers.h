#pragma once
#include "../audio_expression.h"
#include "../audio_fade.h"
#include "../audio_sub_object.h"

namespace Halley {
	class AudioSubObjectLayers final : public IAudioSubObject {
	public:
		struct Layer {
			AudioSubObjectHandle object;
			AudioExpression expression;
			std::optional<AudioFade> fadeIn;
			std::optional<AudioFade> fadeOut;
			bool synchronised = false;

			Layer() = default;
			Layer(const ConfigNode& node);
			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};

		void load(const ConfigNode& node) override;
		ConfigNode toConfigNode() const override;

		AudioSubObjectType getType() const override { return AudioSubObjectType::Layers; }

		String getName() const override;
		size_t getNumSubObjects() const override;
		AudioSubObjectHandle& getSubObject(size_t n) override;
		bool canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const override;
		void addObject(AudioSubObjectHandle handle, const std::optional<String>& caseName, size_t idx) override;
		AudioSubObjectHandle removeObject(const IAudioObject* object) override;

		std::unique_ptr<AudioSource> makeSource(AudioEngine& engine, AudioEmitter& emitter) const override;
		void loadDependencies(Resources& resources) override;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		const Layer& getLayer(size_t idx) const;

		gsl::span<Layer> getLayers();
		AudioFade& getFade();

	private:
		Vector<Layer> layers;
		AudioFade fadeConfig;
	};
}
