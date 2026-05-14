#pragma once
#include "../ui_widget.h"
#include "halley/graphics/camera.h"
#include "halley/graphics/mesh/mesh_renderer.h"

namespace Halley {
	class RenderSurface;

	class UIScene3D : public UIWidget {
	public:
		explicit UIScene3D(String id, const HalleyAPI& api, Resources& resources, Colour4f bgCol = Colour4f(0.1f, 0.1f, 0.1f));

		void update(Time t, bool moved) override;

		bool hasRender() const override;
		void draw(UIPainter& painter) const override;
		void render(RenderContext& rc) const override;

		std::shared_ptr<const Mesh> loadMesh(const String& meshId);

		void addRenderer(std::unique_ptr<MeshRenderer> renderer);
		void clearRenderers();

		void setCamera(Camera camera);
		Camera& getCamera();
		const Camera& getCamera() const;

		void setOrbitCamera(Vector3f centre, float yaw, float pitch, float distance, float fov = 45.0f);

		void setBGColour(Colour4f colour);
		Colour4f getBGColour() const;

		void setMaterial(const String& material);
		std::shared_ptr<Material> getMaterial();

		Sprite getSurfaceSprite() const;
		
		void setRenderScale(float scale);
		float getRenderScale() const;

	private:
		Resources& resources;
		std::shared_ptr<Material> material;

		Colour4f bgCol;
		Vector<std::unique_ptr<MeshRenderer>> renderers;
		Camera camera;

		std::unique_ptr<RenderSurface> renderSurface;

		float renderScale = 1.0f;

        void drawOnPainter(Painter& painter) const;
    };
}
