#include "graphics/render_target/render_surface.h"
#include "graphics/render_target/render_target_texture.h"
#include "graphics/texture.h"
#include "graphics/texture_descriptor.h"
#include "graphics/material/material.h"
#include "graphics/material/material_definition.h"
#include "api/video_api.h"
#include "graphics/sprite/sprite.h"
#include "resources/resources.h"

using namespace Halley;

RenderSurface::RenderSurface(VideoAPI& video, Resources& resources, const String& materialName)
	: video(video)
{
	material = std::make_shared<Material>(resources.get<MaterialDefinition>(materialName));
}

void RenderSurface::setSize(Vector2i size)
{
	if (size != curRenderSize && size.x > 0 && size.y > 0) {
		curRenderSize = size;

		const auto textureSize = Vector2i(nextPowerOf2(size.x), nextPowerOf2(size.y));
		if (textureSize != curTextureSize) {
			curTextureSize = textureSize;

			std::shared_ptr<Texture> colourTarget = video.createTexture(textureSize);
			auto colourDesc = TextureDescriptor(textureSize, TextureFormat::RGBA);
			colourDesc.isRenderTarget = true;
			colourTarget->load(std::move(colourDesc));
			material->set("tex0", colourTarget);

			std::shared_ptr<Texture> depthTarget = video.createTexture(textureSize);
			auto depthDesc = TextureDescriptor(textureSize, TextureFormat::DEPTH);
			depthDesc.isDepthStencil = true;
			depthTarget->load(std::move(depthDesc));

			renderTarget = video.createTextureRenderTarget();
			renderTarget->setTarget(0, colourTarget);
			renderTarget->setDepthTexture(depthTarget);
		}

		renderTarget->setViewPort(Rect4i(Vector2i(), size));
	}
}

bool RenderSurface::isReady() const
{
	return static_cast<bool>(renderTarget);
}

Sprite RenderSurface::getSurfaceSprite() const
{
	const auto& tex = renderTarget->getTexture(0);
	return Sprite()
		.setMaterial(material)
		.setSize(Vector2f(curRenderSize))
		.setTexRect(Rect4f(Vector2f(), Vector2f(curRenderSize) / Vector2f(tex->getSize())));
}

TextureRenderTarget& RenderSurface::getRenderTarget() const
{
	return *renderTarget;
}
