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

RenderSurface::RenderSurface(VideoAPI& video, RenderSurfaceOptions options)
	: video(video)
	, options(options)
{
}

RenderSurface::RenderSurface(VideoAPI& video, Resources& resources, const String& materialName, RenderSurfaceOptions options)
	: video(video)
	, options(options)
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
			if (!options.name.isEmpty()) {
				colourTarget->setAssetId(options.name + "_colour0_v" + toString(version));
			}
			auto colourDesc = TextureDescriptor(textureSize, TextureFormat::RGBA);
			colourDesc.isRenderTarget = true;
			colourDesc.useFiltering = options.useFiltering;
			colourTarget->load(std::move(colourDesc));

			if (material) {
				material->set(0, colourTarget);
			}

			renderTarget = video.createTextureRenderTarget();
			renderTarget->setTarget(0, colourTarget);

			if (options.createDepthStencil) {
				std::shared_ptr<Texture> depthTarget = video.createTexture(textureSize);
				if (!options.name.isEmpty()) {
					depthTarget->setAssetId(options.name + "_depth_v" + toString(version));
				}
				auto depthDesc = TextureDescriptor(textureSize, TextureFormat::Depth);
				depthDesc.isDepthStencil = true;
				depthTarget->load(std::move(depthDesc));
				renderTarget->setDepthTexture(depthTarget);
			}

			version++;
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
	Expects(material);
	
	const auto& tex = renderTarget->getTexture(0);
	return Sprite()
		.setMaterial(material, false)
		.setSize(Vector2f(curRenderSize))
		.setTexRect(Rect4f(Vector2f(), Vector2f(curRenderSize) / Vector2f(tex->getSize())));
}

TextureRenderTarget& RenderSurface::getRenderTarget() const
{
	return *renderTarget;
}
