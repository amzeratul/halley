#pragma once
#include "entity.h"
#include "halley/bytes/config_node_serializer_base.h"
#include "halley/time/halleytime.h"
#include "halley/support/logger.h"

namespace Halley {
	class DataInterpolatorSet {
	public:
		void setInterpolator(std::unique_ptr<IDataInterpolator> interpolator, EntityId entity, std::string_view componentName, std::string_view fieldName);
		IDataInterpolator* tryGetInterpolator(EntityId entity, std::string_view componentName, std::string_view fieldName);
		bool setInterpolatorEnabled(EntityId entity, std::string_view componentName, std::string_view fieldName, bool enabled);

		void update(Time time) const;

	private:
		using Key = std::tuple<EntityId, std::string_view, std::string_view>;

		Vector<std::pair<Key, std::unique_ptr<IDataInterpolator>>> interpolators; // Vector as hashing this is complex and we only expect a few interpolators per entity

		Key makeKey(EntityId entity, std::string_view componentName, std::string_view fieldName) const;
	};

	class DataInterpolatorSetRetriever : public IDataInterpolatorSet {
	public:
		DataInterpolatorSetRetriever(EntityRef rootEntity);
		
		IDataInterpolator* tryGetInterpolator(const EntitySerializationContext& context, std::string_view componentName, std::string_view fieldName) const override;

	private:
		DataInterpolatorSet* dataInterpolatorSet = nullptr;
	};

	template <typename T>
	class DataInterpolator : public IDataInterpolator {
	public:
		void deserialize(void* value, const void* defaultValue, const EntitySerializationContext& context, const ConfigNode& node) override
		{
			if (enabled) {
				doDeserialize(*static_cast<T*>(value), *static_cast<const T*>(defaultValue), context, node);
			}
		}
		
		void setEnabled(bool enabled) override { this->enabled = enabled; }
		bool isEnabled() const override { return enabled; }

	protected:
		virtual void doDeserialize(T& value, const T& defaultValue, const EntitySerializationContext& context, const ConfigNode& node)
		{
			ConfigNodeHelper<T>::deserialize(value, defaultValue, context, node);
		}

	private:
		bool enabled = true;
	};

	template <typename T, typename Intermediate = T>
	class LerpDataInterpolator : public DataInterpolator<T> {
	public:
		LerpDataInterpolator(Time length) : length(length) {}

		void update(Time t) override
		{
			if (targetValue) {
				const Time stepT = std::min(t, timeLeft);
				if (stepT > 0.0000001) {
					if constexpr (std::is_same_v<T, Intermediate>) {
						*targetValue += static_cast<T>(delta * (stepT / length));
					} else {
						*targetValue = T(static_cast<Intermediate>(*targetValue) + static_cast<Intermediate>(delta * (stepT / length)));
					}
				}
				timeLeft -= stepT;
			}
		}

	protected:
		void doDeserialize(T& value, const T& defaultValue, const EntitySerializationContext& context, const ConfigNode& node) override
		{
			T newValue = value;
			ConfigNodeHelper<T>::deserialize(newValue, defaultValue, context, node);

			if constexpr (std::is_same_v<T, Intermediate>) {
				delta = newValue - value;
			} else {
				delta = Intermediate(newValue) - Intermediate(value);
			}
			timeLeft = length;
			targetValue = &value;
		}

	private:
		const Time length;
		Time timeLeft = 0;
		Intermediate delta;
		T* targetValue = nullptr;
	};
}
