#pragma once
#include "../ui_widget.h"
#include "halley/graphics/camera.h"
#include "halley/graphics/mesh/mesh_renderer.h"

namespace Halley {
	class RenderSurface;

	class UIScene3D : public UIWidget {
	public:
		explicit UIScene3D(String id, const HalleyAPI& api, Resources& resources);

		void update(Time t, bool moved) override;

		bool hasRender() const override;
		void draw(UIPainter& painter) const override;
		void render(RenderContext& rc) const override;

		void addRenderer(std::unique_ptr<MeshRenderer> renderer);
		void clearRenderers();

		void setCamera(Camera camera);
		Camera& getCamera();
		const Camera& getCamera() const;

		void setBGColour(Colour4f colour);
		Colour4f getBGColour() const;

		void setMaterial(const String& material);
		std::shared_ptr<Material> getMaterial();

		Sprite getSurfaceSprite() const;

	private:
		Resources& resources;
		std::shared_ptr<Material> material;

		Colour4f bgCol;
		Vector<std::unique_ptr<MeshRenderer>> renderers;
		Camera camera;

		std::unique_ptr<RenderSurface> renderSurface;

        void drawOnPainter(Painter& painter) const;
    };
}
