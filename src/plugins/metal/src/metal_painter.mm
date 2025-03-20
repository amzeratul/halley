#include "metal_material_constant_buffer.h"
#include "metal_painter.h"
#include "metal_render_target.h"
#include "metal_texture.h"
#include "metal_video.h"
#include <iostream>

using namespace Halley;

MetalPainter::MetalPainter(MetalVideo& video, Resources& resources)
	: Painter(video, resources)
	, video(video)
	, indexBuffer(nil)
	, nextRenderPassDescriptor(nil)
{}

void MetalPainter::doClear(std::optional<Colour> colour, std::optional<float> depth, std::optional<uint8_t> stencil) {

	if(!nextRenderPassDescriptor) {
		throw Exception( "Clearing without bound rendertarget", HalleyExceptions::VideoPlugin);
	}

	if (colour) {
		nextRenderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(colour->r, colour->g, colour->b, colour->a);
		nextRenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
	}

	if (depth && nextRenderPassDescriptor.depthAttachment.texture) {
		nextRenderPassDescriptor.depthAttachment.clearDepth = *depth;
		nextRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
	}

	if (stencil && nextRenderPassDescriptor.stencilAttachment.texture) {
		throw Exception( "Not implemented yet", HalleyExceptions::Graphics );
		//nextRenderPassDescriptor.stencilAttachment.clearStencil = *stencil;
		//nextRenderPassDescriptor.stencilAttachment.loadAction = MTLLoadActionClear;
	}
}

void MetalPainter::setMaterialPass(const Material& material, int passNumber) {
	auto& pass = material.getDefinition().getPass(passNumber);
	MetalShader& shader = static_cast<MetalShader&>(pass.getShader());

	auto pipelineStateDescriptor = shader.setupMaterial(material);
	setBlending(pass.getBlend(), pipelineStateDescriptor.colorAttachments[0]);

	setDepthStencil(material.getDepthStencil(passNumber));

	if( currentRenderTarget && currentRenderTarget->getMetalDepthTexture() ) {
		pipelineStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
	}

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
	static_cast<MetalMaterialConstantBuffer&>(getConstantBuffer(halleyGlobalMaterial->getDataBlocks().front())).bindVertex(encoder, 0);

	// Bind textures
	int textureUnit = 0;
	for (auto& tex : material.getDefinition().getTextures()) {
		auto texture = std::static_pointer_cast<const MetalTexture>(material.getTexture(textureUnit));
		if (!texture) {
			throw Exception("Error binding texture to texture unit #" + toString(textureUnit) + " with material \"" + material.getDefinition().getName() + "\": texture is null.", HalleyExceptions::VideoPlugin);
		}

		texture->bind(encoder, textureUnit);

		++textureUnit;
	}
}

void MetalPainter::doStartRender() {
}

void MetalPainter::doEndRender() {
}

void MetalPainter::startEncoding(IMetalRenderTarget * renderTarget) {
	nextRenderPassDescriptor = renderPassDescriptorForTexture(renderTarget->getMetalTexture(), renderTarget->getMetalDepthTexture());
	currentRenderTarget = renderTarget;
}

void MetalPainter::endEncoding() {
	ensureEncoder(); // :TRICKY: If no draw occured, we still need to clear
	[encoder endEncoding];
	encoder = nil;
}

void MetalPainter::setVertices(
	const MaterialDefinition& material, size_t numVertices, const void* vertexData, size_t numIndices,
	const IndexType* indices, bool standardQuadsOnly
) {
	ensureEncoder();
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
	if( encoder ) {
		int scaleFactor = dynamic_cast<IMetalRenderTarget&>( getActiveRenderTarget() ).getScaleFactor();
		[encoder setViewport:(MTLViewport){
			static_cast<double>(rect.getTopLeft().x * scaleFactor),
			static_cast<double>(rect.getTopLeft().y * scaleFactor),
			static_cast<double>(rect.getWidth() * scaleFactor),
			static_cast<double>(rect.getHeight() * scaleFactor),
			0.0, 1.0
		}];
	} else {
		viewPort = rect;
	}
}

void MetalPainter::setClip(Rect4i rect, bool) {
	Rect4i screenRect = rect.intersection( getActiveRenderTarget().getViewPort() );

	// Tricky: it can happen the position is outside the screen
	// In this case, the intersection will be empty
	// yet position is still outside the area and metal does not like it
	if( screenRect.isEmpty() )
	{
		screenRect.setX(0);
		screenRect.setY(0);
	}
	if( encoder ) {
		int scaleFactor = dynamic_cast<IMetalRenderTarget&>( getActiveRenderTarget() ).getScaleFactor();
		[encoder setScissorRect:(MTLScissorRect){
			static_cast<NSUInteger>(screenRect.getTopLeft().x * scaleFactor),
			static_cast<NSUInteger>(screenRect.getTopLeft().y * scaleFactor),
			static_cast<NSUInteger>(screenRect.getWidth() * scaleFactor),
			static_cast<NSUInteger>(screenRect.getHeight() * scaleFactor)
		}];
	} else {
		clipRect = rect;
	}
}

void MetalPainter::setMaterialData(const Material& material) {


	for (auto& dataBlock : material.getDataBlocks()) {
		if (dataBlock.getType() != MaterialDataBlockType::SharedExternal) {
			int bind_point = dataBlock.getBindPoint();

			static_cast<MetalMaterialConstantBuffer&>(getConstantBuffer(dataBlock)).bindVertex(encoder, bind_point);
			// Tricky : The Vertex binding start at one, but the pixel at 0
			static_cast<MetalMaterialConstantBuffer&>(getConstantBuffer(dataBlock)).bindFragment(encoder, bind_point - 1);
		}
	}
}

void MetalPainter::onUpdateProjection(Material& material, bool hashChanged) {
	if (hashChanged) {
		//:TODO: Check if needed : material.uploadData(*this);
		setMaterialData(material);
	}
}

void MetalPainter::setBlending(BlendType blendType, MTLRenderPipelineColorAttachmentDescriptor* colorAttachment) {
	const auto blendMode = blendType.mode;
	Expects(
		blendMode == BlendMode::Alpha || blendMode == BlendMode::Max || blendMode == BlendMode::Min || blendMode == BlendMode::Add ||
		blendMode == BlendMode::Opaque || blendMode == BlendMode::Multiply || blendMode == BlendMode::Darken
	);
	bool useBlending = blendMode != BlendMode::Opaque;
	colorAttachment.blendingEnabled = useBlending;
	if (!useBlending) {
		return;
	}

	colorAttachment.rgbBlendOperation = MTLBlendOperationAdd;
	colorAttachment.alphaBlendOperation = MTLBlendOperationAdd;

	switch (blendMode) {
		case BlendMode::Alpha:
			setBlendFactor(colorAttachment, blendType.premultiplied ? MTLBlendFactorOne : MTLBlendFactorSourceAlpha, MTLBlendFactorOneMinusSourceAlpha);
			break;
		case BlendMode::Add:
			colorAttachment.sourceRGBBlendFactor = blendType.premultiplied ? MTLBlendFactorOne : MTLBlendFactorSourceAlpha;
			colorAttachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
			colorAttachment.destinationRGBBlendFactor = MTLBlendFactorOne;
			colorAttachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
			break;
		case BlendMode::Multiply:
			setBlendFactor(colorAttachment, MTLBlendFactorDestinationColor, MTLBlendFactorOneMinusSourceAlpha);
			break;
		case BlendMode::Max:
			colorAttachment.sourceRGBBlendFactor = blendType.premultiplied ? MTLBlendFactorOne : MTLBlendFactorSourceAlpha;
			colorAttachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
			colorAttachment.destinationRGBBlendFactor = MTLBlendFactorOne;
			colorAttachment.destinationAlphaBlendFactor = MTLBlendFactorOne;
			colorAttachment.rgbBlendOperation = MTLBlendOperationMax;
			colorAttachment.alphaBlendOperation = MTLBlendOperationMax;
			break;
		default:
			throw Exception("Not implemented yet", HalleyExceptions::Graphics);
	}
}

void MetalPainter::setBlendFactor(MTLRenderPipelineColorAttachmentDescriptor* colorAttachment, MTLBlendFactor src, MTLBlendFactor dst) {
	colorAttachment.sourceRGBBlendFactor = src;
	colorAttachment.sourceAlphaBlendFactor = src;
	colorAttachment.destinationRGBBlendFactor = dst;
	colorAttachment.destinationAlphaBlendFactor = dst;
}

MTLRenderPassDescriptor* MetalPainter::renderPassDescriptorForTexture(id<MTLTexture> texture, id<MTLTexture> depthTexture) {
	MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
	pass.colorAttachments[0].loadAction = MTLLoadActionLoad;
	pass.colorAttachments[0].storeAction = MTLStoreActionStore;
	pass.colorAttachments[0].texture = texture;

	if (depthTexture != nil) {
		pass.depthAttachment.loadAction = MTLLoadActionLoad;
		pass.depthAttachment.storeAction = MTLStoreActionStore;
		pass.depthAttachment.texture = depthTexture;
	}

	return pass;
}

void MetalPainter::ensureEncoder()
{
	if( encoder ) return;
	encoder = [video.getCommandBuffer() renderCommandEncoderWithDescriptor:nextRenderPassDescriptor];
	nextRenderPassDescriptor = nil;

	if( viewPort ) {
		auto & rect = *viewPort;
		int scaleFactor = dynamic_cast<IMetalRenderTarget&>( getActiveRenderTarget() ).getScaleFactor();
		[encoder setViewport:(MTLViewport){
			static_cast<double>(rect.getTopLeft().x * scaleFactor),
			static_cast<double>(rect.getTopLeft().y * scaleFactor),
			static_cast<double>(rect.getWidth() * scaleFactor),
			static_cast<double>(rect.getHeight() * scaleFactor),
			0.0, 1.0
		}];
		viewPort.reset();
	}

	if( clipRect ) {
		auto & rect = *clipRect;
		int scaleFactor = dynamic_cast<IMetalRenderTarget&>( getActiveRenderTarget() ).getScaleFactor();
		[encoder setScissorRect:(MTLScissorRect){
			static_cast<NSUInteger>(rect.getTopLeft().x * scaleFactor),
			static_cast<NSUInteger>(rect.getTopLeft().y * scaleFactor),
			static_cast<NSUInteger>(rect.getWidth() * scaleFactor),
			static_cast<NSUInteger>(rect.getHeight() * scaleFactor)
		}];
		clipRect.reset();
	}

	if( curDepthStencil ) {
		curDepthStencil->bind( encoder );
	}
}

MetalDepthStencil& MetalPainter::getDepthStencil(const MaterialDepthStencil& depthStencilDefinition)
{
	const auto iter = depthStencils.find(depthStencilDefinition);
	if (iter == depthStencils.end()) {
		auto depthStencil = std::make_unique<MetalDepthStencil>(video, depthStencilDefinition);
		const auto result = depthStencil.get();
		depthStencils[depthStencilDefinition] = std::move(depthStencil);
		return *result;
	}

	return *iter->second;
}

void MetalPainter::setDepthStencil(const MaterialDepthStencil& depthStencilDefinition)
{
	if (!curDepthStencil || curDepthStencil->getDefinition() != depthStencilDefinition) {
		curDepthStencil = &getDepthStencil(depthStencilDefinition);
		curDepthStencil->bind( encoder );
	}
}
