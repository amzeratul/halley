#include <systems/particle_system.h>

using namespace Halley;

class ParticleSystem final : public ParticleSystemBase<ParticleSystem>, public IParticleSpawner {
public:
	void onEntitiesAdded(Span<ParticleFamily> es)
	{
		for (auto& e: es) {
			refreshParticles(e);
		}
	}

	void onEntitiesReloaded(Span<ParticleFamily*> es)
	{
		for (auto& e : es) {
			refreshParticles(*e);
		}
	}

	void update(Time t)
	{
		for (auto& e: particleFamily) {
			auto& particles = e.particles.particles;
			particles.setPosition(Vector3f(e.transform2D.getGlobalPosition(), e.transform2D.getGlobalHeight()));
			particles.setSecondarySpawner(this);
			particles.update(t);

			if (const auto aabb = particles.getAABB(); aabb && getScreenService().isVisible(*aabb)) {
				particles.updateSprites(t);
			}

			if (!particles.isAlive() && !particles.isEnabled() && !getWorld().isEditor()) {
				getWorld().destroyEntity(e.entityId);
			}
		}

		editorDebugDraw();
	}

	void onMessageReceived(const StopParticlesMessage& msg, ParticleFamily& particles) override
	{
		particles.particles.particles.setEnabled(false);
	}

	void onMessageReceived(const DirectionalBurstMessage& msg, ParticleFamily& particles) override
	{
		particles.particles.particles.directionalBurst(msg.direction.angle(), msg.count);
	}

	void spawn(Vector3f pos, EntityId target) override
	{
		// TODO: this could be a perf bottleneck
		// Would it be faster to tryGetEntity(), then tryGetComponent()?
		if (auto* particles = particleFamily.tryFind(target)) {
			particles->particles.particles.spawnTriggeredByAnotherSystem(pos);
		}
	}

private:
	void refreshParticles(ParticleFamily& e)
	{
		auto& particles = e.particles.particles;
		particles.setSprites(e.particles.sprites);
		if (e.particles.animation) {
			particles.setAnimation(e.particles.animation.get(), e.particles.sequence, e.particles.direction);
		}
		particles.reset();
	}

	void editorDebugDraw()
	{
		for (const auto& sel: getDevService().getSelectedEntities()) {
			if (const auto* e = particleFamily.tryFind(sel)) {
				drawParticleEntity(*e);
			}
		}
	}

	void drawParticleEntity(const ParticleFamily& e)
	{
		const auto& ps = e.particles.particles;
		const auto area = ps.getSpawnArea();
		const auto pos = e.transform2D.getGlobalPosition();
		if (ps.getSpawnAreaShape() == ParticleSpawnAreaShape::Rectangle) {
			getDebugDrawService().addDebugLine(Polygon::makePolygon(pos - area / 2, area.x, area.y).getVertices(), Colour4f(0.8f, 1.0f, 1.0f), 1.0f, true);
		} else {
			getDebugDrawService().addDebugEllipse(pos, area * 0.5f, Colour4f(0.8f, 1.0f, 1.0f), 1.0f);
		}
	}

};

REGISTER_SYSTEM(ParticleSystem)

