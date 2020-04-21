#include "metal_material_constant_buffer.h"
#include "metal_painter.h"
#include "metal_render_target.h"
#include "metal_texture.h"
#include "metal_video.h"

using namespace Halley;

MetalPainter::MetalPainter(MetalVideo& video, Resources& resources)
	: Painter(resources)
	, video(video)
	, indexBuffer(nil)
{}

void MetalPainter::clear(Colour colour) {
	[encoder endEncoding];
	auto& renderTarget = dynamic_cast<IMetalRenderTarget&>(getActiveRenderTarget());
	auto descriptor = renderPassDescriptorForTextureAndColour(renderTarget.getMetalTexture(), colour);
	encoder = [video.getCommandBuffer() renderCommandEncoderWithDescriptor:descriptor];
}

void MetalPainter::setMaterialPass(const Material& material, int passNumber) {
	auto& pass = material.getDefinition().getPass(passNumber);
	MetalShader& shader = static_cast<MetalShader&>(pass.getShader());

	auto pipelineStateDescriptor = shader.setupMaterial(material);
	setBlending(pass.getBlend(), pipelineStateDescriptor.colorAttachments[0]);

	NSError* error = NULL;
	id<MTLRenderPipelineState> pipelineState = [[video.getDevice() newRenderPipelineStateWithDescriptor:pipelineStateDescriptor
			error:&error
	] autorelease];
	if (!pipelineState) {
		std::cout << "Failed to create pipeline descriptor for material " << material.getDefinition().getName() <<
			", pass " << passNumber << "." << std::endl;
		throw Exception([[error localizedDescription] UTF8String], HalleyExceptions::VideoPlugin);
	}

	[encoder setRenderPipelineState:pipelineState];

	// Metal requires the global material to be bound for each material pass, as it has no 'global' state.
	static_cast<MetalMaterialConstantBuffer&>(halleyGlobalMaterial->getDataBlocks().front().getConstantBuffer()).bindVertex(encoder, 0);

	// Bind textures
	int texIndex = 0;
	for (auto& tex : material.getTextures()) {
		auto texture = std::static_pointer_cast<const MetalTexture>(tex);
		texture->bind(encoder, texIndex++);
	}
}

void MetalPainter::doStartRender() {
}

void MetalPainter::doEndRender() {
}

void MetalPainter::startEncoding(id<MTLTexture> texture) {
	auto col = Colour4f(0);
	auto descriptor = renderPassDescriptorForTextureAndColour(texture, col);
	encoder = [video.getCommandBuffer() renderCommandEncoderWithDescriptor:descriptor];
}

void MetalPainter::endEncoding() {
	[encoder endEncoding];
}

void MetalPainter::setVertices(
	const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices,
	unsigned short* indices, bool standardQuadsOnly
) {
	Expects(numVertices > 0);
	Expects(numIndices >= numVertices);
	Expects(vertexData);
	Expects(indices);

	size_t bytesSize = numVertices * material.getVertexStride();
	id<MTLBuffer> buffer = [[video.getDevice() newBufferWithBytes:vertexData
		length:bytesSize
		options:MTLResourceStorageModeShared
	] autorelease];
	[encoder setVertexBuffer:buffer offset:0 atIndex:MaxMetalBufferIndex];

	if (indexBuffer != nil) {
		[indexBuffer autorelease];
	}
	indexBuffer = [video.getDevice() newBufferWithBytes:indices
			length:numIndices*sizeof(short) options:MTLResourceStorageModeShared
	];
}

void MetalPainter::drawTriangles(size_t numIndices) {
	Expects(numIndices > 0);
	Expects(numIndices % 3 == 0);

	[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
		indexCount:numIndices
		indexType:MTLIndexTypeUInt16
		indexBuffer:indexBuffer
		indexBufferOffset:0
	];
}

void MetalPainter::setViewPort(Rect4i rect) {
	[encoder setViewport:(MTLViewport){
		static_cast<double>(rect.getTopLeft().x),
		static_cast<double>(rect.getTopLeft().y),
		static_cast<double>(rect.getWidth()),
		static_cast<double>(rect.getHeight()),
		0.0, 1.0
	}];
}

void MetalPainter::setClip(Rect4i rect, bool) {
	[encoder setScissorRect:(MTLScissorRect){
		static_cast<NSUInteger>(rect.getTopLeft().x),
		static_cast<NSUInteger>(rect.getTopLeft().y),
		static_cast<NSUInteger>(rect.getWidth()),
		static_cast<NSUInteger>(rect.getHeight())
	}];
}

void MetalPainter::setMaterialData(const Material& material) {
	for (auto& dataBlock : material.getDataBlocks()) {
		if (dataBlock.getType() != MaterialDataBlockType::SharedExternal) {
			static_cast<MetalMaterialConstantBuffer&>(dataBlock.getConstantBuffer()).bindFragment(encoder, 0);
		}
	}
}

void MetalPainter::onUpdateProjection(Material& material) {
	material.uploadData(*this);
	setMaterialData(material);
}

void MetalPainter::setBlending(BlendType blendType, MTLRenderPipelineColorAttachmentDescriptor* colorAttachment) {
	Expects(
		blendType == BlendType::Alpha || blendType == BlendType::AlphaPremultiplied || blendType == BlendType::Add ||
		blendType == BlendType::Opaque || blendType == BlendType::Multiply || blendType == BlendType::Darken
	);
	bool useBlending = blendType != BlendType::Opaque;
	colorAttachment.blendingEnabled = useBlending;
	if (!useBlending) {
		return;
	}

	colorAttachment.rgbBlendOperation = MTLBlendOperationAdd;
	colorAttachment.alphaBlendOperation = MTLBlendOperationAdd;

	switch (blendType) {
		case BlendType::Alpha:
			setBlendFactor(colorAttachment, MTLBlendFactorSourceAlpha, MTLBlendFactorOneMinusSourceAlpha);
			break;
		case BlendType::AlphaPremultiplied:
			setBlendFactor(colorAttachment, MTLBlendFactorOne, MTLBlendFactorOneMinusSourceAlpha);
			break;
		case BlendType::Multiply:
			setBlendFactor(colorAttachment, MTLBlendFactorDestinationColor, MTLBlendFactorOneMinusSourceAlpha);
			break;
		default:
			setBlendFactor(colorAttachment, MTLBlendFactorSourceAlpha, MTLBlendFactorOne);
	}
}

void MetalPainter::setBlendFactor(MTLRenderPipelineColorAttachmentDescriptor* colorAttachment, MTLBlendFactor src, MTLBlendFactor dst) {
	colorAttachment.sourceRGBBlendFactor = src;
	colorAttachment.sourceAlphaBlendFactor = src;
	colorAttachment.destinationRGBBlendFactor = dst;
	colorAttachment.destinationAlphaBlendFactor = dst;
}

MTLRenderPassDescriptor* MetalPainter::renderPassDescriptorForTextureAndColour(id<MTLTexture> texture, Colour& colour) {
	MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
	pass.colorAttachments[0].clearColor = MTLClearColorMake(colour.r, colour.g, colour.b, colour.a);
	pass.colorAttachments[0].loadAction = MTLLoadActionClear;
	pass.colorAttachments[0].storeAction = MTLStoreActionStore;
	pass.colorAttachments[0].texture = texture;
	return pass;
}
