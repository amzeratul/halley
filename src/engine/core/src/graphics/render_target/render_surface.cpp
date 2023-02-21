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
	renderTarget = video.createTextureRenderTarget();
}

RenderSurface::RenderSurface(VideoAPI& video, Resources& resources, const String& materialName, RenderSurfaceOptions options)
	: video(video)
	, options(options)
{
	material = std::make_shared<Material>(resources.get<MaterialDefinition>(materialName));
	renderTarget = video.createTextureRenderTarget();
}

void RenderSurface::setSize(Vector2i size)
{
	bool needsNewBuffers = false;
	const bool hasValidSize = size.x > 0 && size.y > 0;
	
	if (size != curRenderSize) {
		curRenderSize = size;

		if (hasValidSize) {
			const auto textureSize = options.powerOfTwo ? Vector2i(nextPowerOf2(size.x), nextPowerOf2(size.y)) : size;
			if (textureSize != curTextureSize) {
				curTextureSize = textureSize;
				needsNewBuffers = true;
			}
		}

		renderTarget->setViewPort(Rect4i(Vector2i(), size));
	}

	if (hasValidSize) {
		if (needsNewBuffers || !hasColourTarget) {
			createNewColourTarget();
		}
		if (options.createDepthStencil && (needsNewBuffers || !hasDepthStencil)) {
			createNewDepthStencilTarget();
		}
	}
}

bool RenderSurface::isReady() const
{
	return hasColourTarget && (!options.createDepthStencil || hasDepthStencil);
}

Sprite RenderSurface::getSurfaceSprite() const
{
	return getSurfaceSprite(material);
}

Sprite RenderSurface::getSurfaceSprite(std::shared_ptr<Material> material) const
{
	Expects(material);
	
	const auto& tex = renderTarget->getTexture(0);
	material->set(0, tex);
	return Sprite()
		.setMaterial(material)
		.setSize(Vector2f(curRenderSize))
		.setTexRect(Rect4f(Vector2f(), Vector2f(curRenderSize) / Vector2f(tex->getSize())));
}

TextureRenderTarget& RenderSurface::getRenderTarget() const
{
	return *renderTarget;
}

void RenderSurface::createNewColourTarget()
{
	std::shared_ptr<Texture> colourTarget = video.createTexture(curTextureSize);
	if (!options.name.isEmpty()) {
		colourTarget->setAssetId(options.name + "_colour0_v" + toString(version));
	}
	auto colourDesc = TextureDescriptor(curTextureSize, options.colourBufferFormat);
	colourDesc.isRenderTarget = true;
	colourDesc.useFiltering = options.useFiltering;
	colourDesc.useMipMap = options.mipMap;
	colourTarget->load(std::move(colourDesc));

	setColourTarget(std::move(colourTarget));
}

void RenderSurface::createNewDepthStencilTarget()
{
	std::shared_ptr<Texture> depthTarget = video.createTexture(curTextureSize);
	if (!options.name.isEmpty()) {
		depthTarget->setAssetId(options.name + "_depth_v" + toString(version));
	}
	auto depthDesc = TextureDescriptor(curTextureSize, TextureFormat::Depth);
	depthDesc.isDepthStencil = true;
	depthTarget->load(std::move(depthDesc));

	setDepthStencilTarget(std::move(depthTarget));
}

void RenderSurface::setColourTarget(std::shared_ptr<Texture> texture)
{
	hasColourTarget = !!texture;
	if (texture) {
		if (texture->getSize() == curTextureSize) {
			renderTarget->setTarget(0, std::move(texture));
		} else {
			createNewColourTarget();
		}
	}
	version++;
}

void RenderSurface::setDepthStencilTarget(std::shared_ptr<Texture> texture)
{
	hasDepthStencil = !!texture;
	if (texture) {
		if (texture->getSize() == curTextureSize) {
			renderTarget->setDepthTexture(std::move(texture));
		} else {
			createNewDepthStencilTarget();
		}
	}
	version++;
}
