#pragma once
#include <halley/core/graphics/painter.h>
#include <Metal/Metal.h>

namespace Halley {
	class MetalVideo;

	static inline MTLRenderPassDescriptor* renderPassDescriptorForTextureAndColour(id<MTLTexture> texture, Colour& colour) {
		MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
		pass.colorAttachments[0].clearColor = MTLClearColorMake(colour.r, colour.g, colour.b, colour.a);
		pass.colorAttachments[0].loadAction = MTLLoadActionClear;
		pass.colorAttachments[0].storeAction = MTLStoreActionStore;
		pass.colorAttachments[0].texture = texture;
		return pass;
	}

	class MetalPainter : public Painter
	{
	public:
		explicit MetalPainter(MetalVideo& video, Resources& resources);
		void clear(Colour colour) override;
		void setMaterialPass(const Material& material, int pass) override;
		void doStartRender() override;
		void doEndRender() override;
		void setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices, bool standardQuadsOnly) override;
		void drawTriangles(size_t numIndices) override;
		void setViewPort(Rect4i rect) override;
		void setClip(Rect4i clip, bool enable) override;
		void setMaterialData(const Material& material) override;
		void onUpdateProjection(Material& material) override;

	private:
		void setBlending(BlendType blendType, MTLRenderPipelineColorAttachmentDescriptor* colorAttachment);
		void setBlendFactor(MTLRenderPipelineColorAttachmentDescriptor* colorAttachment, MTLBlendFactor src, MTLBlendFactor dst);

		MetalVideo& video;
		id<MTLCommandBuffer> buffer;
		id<MTLRenderCommandEncoder> encoder;
		id<MTLBuffer> indexBuffer;
	};
}
