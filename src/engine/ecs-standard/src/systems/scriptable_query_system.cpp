#include <systems/scriptable_query_system.h>

using namespace Halley;

class ScriptableQuerySystem final : public ScriptableQuerySystemBase<ScriptableQuerySystem>, IScriptableQuerySystemInterface {
public:
	void init()
	{
		getWorld().setInterface<IScriptableQuerySystemInterface>(this);
	}

	void update(Time t)
	{
	}

	Vector<EntityId> findEntities(WorldPosition pos, float radius, int limit, const Vector<String>& tags, const Vector<String>& components, const std::function<float(EntityId, WorldPosition)>& getDistance) const override
	{
		Vector<ComponentReflector*> reflectors;
		for (const auto& component: components) {
			reflectors.push_back(&getWorld().getReflection().getComponentReflector(component));
		}

		// Collect all matching
		using T = std::pair<float, EntityId>;
		Vector<T> matching;
		Vector<EntityId> matchId;
		auto checkEntity = [&] (EntityId entityId, gsl::span<const String> entityTags)
		{
			if (!std_ex::contains(matchId, entityId)) {
				if (std::all_of(tags.begin(), tags.end(), [&](const String& tag) { return std_ex::contains(entityTags, tag); })) {
					if (!reflectors.empty()) {
						auto e = getWorld().getEntity(entityId);
						for (const auto& r: reflectors) {
							if (r->tryGetComponent(e) == nullptr) {
								return;
							}
						}
					}

					const float distance = getDistance(entityId, pos);
					if (distance <= radius && std::isfinite(distance)) {
						matching.emplace_back(distance, entityId);
						matchId.push_back(entityId);
					}
				}
			}
		};
		for (const auto& e: scriptableFamily) {
			checkEntity(e.entityId, e.scriptable.tags);
		}
		for (const auto& e: tagTargetsFamily) {
			checkEntity(e.entityId, e.scriptTagTarget.tags);
		}

		if (callback) {
			for (const auto& [entityId, entityTags]: callback()) {
				checkEntity(entityId, entityTags);
			}
		}

		// Sort by proximity
		std::sort(matching.begin(), matching.end(), [](const T& a, const T& b) -> bool
		{
			return a.first < b.first;
		});

		// Prune excess
		if (matching.size() > limit) {
			matching.resize(limit);
		}

		// Convert
		Vector<EntityId> result;
		result.reserve(matching.size());
		for (const auto& e: matching) {
			result.push_back(e.second);
		}
		return result;
	}

	void setFindEntitiesCallback(Callback callback) override
	{
		this->callback = callback;
	}

private:
	Callback callback;
};

REGISTER_SYSTEM(ScriptableQuerySystem)

