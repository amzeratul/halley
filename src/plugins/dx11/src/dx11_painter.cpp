#include "dx11_painter.h"
#include "dx11_video.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "dx11_shader.h"
#include "dx11_material_constant_buffer.h"
#include "dx11_blend.h"
#include "halley/graphics/material/material_parameter.h"
#include "dx11_texture.h"
#include "dx11_rasterizer.h"
#include "halley/graphics/render_target/render_target.h"
#include "dx11_render_target.h"
#include "halley/game/game_platform.h"
#include "dx11_depth_stencil.h"
#include "halley/graphics/render_snapshot.h"
#include "halley/support/logger.h"
using namespace Halley;

DX11Painter::DX11Painter(DX11Video& video, Resources& resources)
	: Painter(video, resources)
	, dx11Video(video)
{
#ifdef WINDOWS_STORE
	// Due to the architecture of "some" platforms available here, updating a dynamic buffer is extremely slow if it's still in use
	constexpr size_t numBuffers = 2;
#else
	constexpr size_t numBuffers = 1;
#endif
	for (size_t i = 0; i < numBuffers; ++i) {
		vertexBuffers.emplace_back(video, DX11Buffer::Type::Vertex, 8 * 1024 * 1024);
		indexBuffers.emplace_back(video, DX11Buffer::Type::Index, 128 * 1024);
	}
}

void DX11Painter::resetState()
{
	Painter::resetState();
	curRaster = nullptr;
	curDepthStencil = nullptr;
	unbindRenderTargetTextureUnits(renderTargetTextureUnits.size(), 0);
}

void DX11Painter::doStartRender()
{
}

void DX11Painter::doEndRender()
{
	// Unbind recently bound render-target textures
	unbindRenderTargetTextureUnits(renderTargetTextureUnits.size(), 0);
	dx11Video.getDeviceContext().Flush();
}

void DX11Painter::doClear(std::optional<Colour> colour, std::optional<float> depth, std::optional<uint8_t> stencil)
{
	flush();
	auto& renderTarget = dynamic_cast<IDX11RenderTarget&>(getActiveRenderTarget());
	
	if (colour) {
		const auto& c = colour.value();
		const float col[] = { c.r, c.g, c.b, c.a };
		dx11Video.getDeviceContext().ClearRenderTargetView(renderTarget.getRenderTargetView(), col);
	}

	if (depth || stencil) {
		auto* depthStencilView = renderTarget.getDepthStencilView();
		if (depthStencilView) {
			UINT flags = 0;
			if (depth) {
				flags |= D3D11_CLEAR_DEPTH;
			}
			if (stencil) {
				flags |= D3D11_CLEAR_STENCIL;
			}
			dx11Video.getDeviceContext().ClearDepthStencilView(depthStencilView, flags, depth.value_or(1.0f), stencil.value_or(0));
		}
	}
}

void DX11Painter::setMaterialPass(const Material& material, int passN)
{
	auto& pass = material.getDefinition().getPass(passN);

	// Raster and depthStencil
	setRasterizer(pass);
	setDepthStencil(material.getDepthStencil(passN));

	// Shader
	auto& shader = static_cast<DX11Shader&>(pass.getShader());
	shader.setMaterialLayout(dx11Video, material.getDefinition().getVertexAttributes());
	shader.bind(dx11Video);

	// Blend
	getBlendMode(pass.getBlend()).bind(dx11Video);

	// Texture
	int textureUnit = 0;
	const size_t numRenderTargetTextureUnits = renderTargetTextureUnits.size();
	for (auto& tex: material.getDefinition().getTextures()) {
		const auto texture = std::static_pointer_cast<const DX11Texture>(material.getTexture(textureUnit));
		if (!texture) {
			throw Exception("Error binding texture to texture unit #" + toString(textureUnit) + " with material \"" + material.getDefinition().getName() + "\": texture is null.", HalleyExceptions::VideoPlugin);
		}

		texture->bind(dx11Video, textureUnit, tex.getSamplerType());
		if (texture->getDescriptor().isRenderTarget) {
			// Remember units for textures which are also render targets
			renderTargetTextureUnits.push_back(textureUnit);
		}
		++textureUnit;
	}
	unbindRenderTargetTextureUnits(numRenderTargetTextureUnits, textureUnit);
}

void DX11Painter::setMaterialData(const Material& material)
{
	auto& devCon = dx11Video.getDeviceContext();
	for (auto& block: material.getDataBlocks()) {
		if (block.getType() != MaterialDataBlockType::SharedExternal) {
			auto& buffer = static_cast<DX11MaterialConstantBuffer&>(getConstantBuffer(block)).getBuffer();
			auto dxBuffer = buffer.getBuffer();

			const auto vsBindPoint = block.getBindPoint(ShaderType::Vertex);
			const auto psBindPoint = block.getBindPoint(ShaderType::Pixel);

			if (Halley::getPlatform() == GamePlatform::UWP || Halley::getPlatform() == GamePlatform::XboxOne) {
				UINT firstConstant[] = { buffer.getOffset() / 16 };
				UINT numConstants[] = { buffer.getLastSize() / 16 };
				if (vsBindPoint >= 0) {
					devCon.VSSetConstantBuffers1(vsBindPoint, 1, &dxBuffer, firstConstant, numConstants);
				}
				if (psBindPoint >= 0) {
					devCon.PSSetConstantBuffers1(psBindPoint, 1, &dxBuffer, firstConstant, numConstants);
				}
			} else {
				if (vsBindPoint >= 0) {
					devCon.VSSetConstantBuffers(vsBindPoint, 1, &dxBuffer);
				}
				if (psBindPoint >= 0) {
					devCon.PSSetConstantBuffers(psBindPoint, 1, &dxBuffer);
				}
			}
		}
	}
}

void DX11Painter::setVertices(const MaterialDefinition& material, size_t numVertices, const void* vertexData, size_t numIndices, const IndexType* indices, bool standardQuadsOnly)
{
	const size_t stride = material.getVertexStride();
	const size_t vertexDataSize = stride * numVertices;

	if (!vertexBuffers[curBuffer].canFit(vertexDataSize) || !indexBuffers[curBuffer].canFit(numIndices * sizeof(unsigned short))) {
		rotateBuffers();
	}

	{
		auto& vb = vertexBuffers[curBuffer];
		vb.setData(gsl::span<const gsl::byte>(reinterpret_cast<const gsl::byte*>(vertexData), vertexDataSize));
		ID3D11Buffer* buffers[] = { vb.getBuffer() };
		UINT strides[] = { UINT(stride) };
		UINT offsets[] = { vb.getOffset() };
		dx11Video.getDeviceContext().IASetVertexBuffers(0, 1, buffers, strides, offsets);
	}

	{
		auto& ib = indexBuffers[curBuffer];
		ib.setData(gsl::as_bytes(gsl::span<const unsigned short>(indices, numIndices)));
		dx11Video.getDeviceContext().IASetIndexBuffer(ib.getBuffer(), DXGI_FORMAT_R16_UINT, ib.getOffset());
	}
}

void DX11Painter::drawTriangles(size_t numIndices)
{
	auto& devCon = dx11Video.getDeviceContext();
	devCon.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devCon.DrawIndexed(UINT(numIndices), 0, 0);
}

void DX11Painter::setViewPort(Rect4i rect)
{
	auto fRect = Rect4f(rect);

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = fRect.getTopLeft().x;
	viewport.TopLeftY = fRect.getTopLeft().y;
	viewport.Width = fRect.getWidth();
	viewport.Height = fRect.getHeight();
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	dx11Video.getDeviceContext().RSSetViewports(1, &viewport);
}

void DX11Painter::setClip(Rect4i clip, bool enable)
{
	clipping = enable ? clip : std::optional<Rect4i>();
}

void DX11Painter::onUpdateProjection(Material& material, bool hashChanged)
{
	if (true || hashChanged) {
		setMaterialData(material);
	}
}

void DX11Painter::onFinishRender()
{
	doCheckEndOfPerformanceMeasurement();
}

DX11Blend& DX11Painter::getBlendMode(BlendType type)
{
	auto iter = blendModes.find(type);
	if (iter != blendModes.end()) {
		return iter->second;
	}

	blendModes.emplace(std::make_pair(type, DX11Blend(dx11Video, type)));
	return getBlendMode(type);
}

void DX11Painter::rotateBuffers()
{
	Expects (vertexBuffers.size() == indexBuffers.size());
	curBuffer = (curBuffer + 1) % vertexBuffers.size();
	indexBuffers[curBuffer].reset();
	vertexBuffers[curBuffer].reset();
}

DX11Rasterizer& DX11Painter::getRasterizer(const DX11RasterizerOptions& options)
{
	auto iter = rasterizers.find(options);
	if (iter != rasterizers.end()) {
		return *iter->second;
	}
	rasterizers[options] = std::make_unique<DX11Rasterizer>(dx11Video, options);
	return *rasterizers[options];
}

void DX11Painter::setRasterizer(const MaterialPass& pass)
{
	DX11RasterizerOptions options;
	options.scissor = static_cast<bool>(clipping);
	options.culling = pass.getCulling();
	setRasterizer(options);
}

void DX11Painter::setRasterizer(const DX11RasterizerOptions& options)
{
	if (!curRaster || curRaster->getOptions() != options) {
		auto& raster = getRasterizer(options);
		curRaster = &raster;
		raster.bind(dx11Video);	
	}

	if (options.scissor) {
		auto clip = clipping.value();
		D3D11_RECT rect;
		rect.top = clip.getTop();
		rect.bottom = clip.getBottom();
		rect.left = clip.getLeft();
		rect.right = clip.getRight();
		dx11Video.getDeviceContext().RSSetScissorRects(1, &rect);
	}
}

DX11DepthStencil& DX11Painter::getDepthStencil(const MaterialDepthStencil& depthStencilDefinition)
{
	const auto iter = depthStencils.find(depthStencilDefinition);
	if (iter == depthStencils.end()) {
		auto depthStencil = std::make_unique<DX11DepthStencil>(dx11Video, depthStencilDefinition);
		const auto result = depthStencil.get();
		depthStencils[depthStencilDefinition] = std::move(depthStencil);
		return *result;
	}
	
	return *iter->second;
}

void DX11Painter::setDepthStencil(const MaterialDepthStencil& depthStencilDefinition)
{
	if (!curDepthStencil || curDepthStencil->getDefinition() != depthStencilDefinition) {
		curDepthStencil = &getDepthStencil(depthStencilDefinition);
		curDepthStencil->bind();
	}
}

void DX11Painter::unbindRenderTargetTextureUnits(size_t lastIndex, int minimumTextureUnit)
{
	// Iterate texture units *previously* bound to render target textures. Unbind
	// only if the texture unit is >= lastIndex.
	// Effectively, this only calls PSSetShaderResources() if the current material
	// pass does not use the texture unit anymore.
	for (size_t idx = 0; idx < lastIndex; ++idx) {
		int textureUnit = renderTargetTextureUnits[idx];
		if (textureUnit >= minimumTextureUnit) {
			ID3D11ShaderResourceView* null_views[] = { nullptr };
			dx11Video.getDeviceContext().PSSetShaderResources(textureUnit, 1, null_views);
		}
	}
	// Remove units stored from the previous pass. Keep values of the current pass.
	if (lastIndex > 0) {
		const auto iter = renderTargetTextureUnits.begin();
		renderTargetTextureUnits.erase(iter, iter + lastIndex);
	}
}

bool DX11Painter::startPerformanceMeasurement()
{
	D3D11_QUERY_DESC desc;
	desc.MiscFlags = 0;
	desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	dx11Video.getDevice().CreateQuery(&desc, &timestampDisjointQuery);
	if (timestampDisjointQuery) {
		dx11Video.getDeviceContext().Begin(timestampDisjointQuery);
		return true;
	}
	return false;
}

void DX11Painter::endPerformanceMeasurement()
{
	dx11Video.getDeviceContext().End(timestampDisjointQuery);
	dx11Video.getDeviceContext().Flush();
}

void DX11Painter::doRecordTimestamp(TimestampType type, size_t id, ITimestampRecorder* snapshot)
{
	ID3D11Query* query = nullptr;
	D3D11_QUERY_DESC desc;
	desc.MiscFlags = 0;
	desc.Query = D3D11_QUERY_TIMESTAMP;
	dx11Video.getDevice().CreateQuery(&desc, &query);

	if (query) {
		dx11Video.getDeviceContext().End(query);
		perfQueries.emplace_back(PerfQuery{ type, id, snapshot, query });
	}

	if (type == TimestampType::FrameStart) {
		dx11Video.getDeviceContext().Flush();
	}
}

void DX11Painter::doCheckEndOfPerformanceMeasurement()
{
	auto clearQueries = [&]()
	{
		for (const auto& q : perfQueries) {
			q.query->Release();
		}
		perfQueries.clear();
	};

	if (timestampDisjointQuery) {
		D3D10_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
		HRESULT result;
		do {
			result = dx11Video.getDeviceContext().GetData(timestampDisjointQuery, &disjointData, sizeof(disjointData), 0);
			if (result != S_OK) {
				std::this_thread::yield();
			}
		} while (result != S_OK);

		timestampDisjointQuery->Release();
		timestampDisjointQuery = nullptr;

		if (disjointData.Disjoint && !perfQueries.empty()) {
			Logger::logWarning("DX11 Profile Data was disjoint");
			clearQueries();
			return;
		}

		UINT64 firstTick = 0;
		for (const auto& q: perfQueries) {
			UINT64 tick;
			do {
				result = dx11Video.getDeviceContext().GetData(q.query, &tick, sizeof(tick), 0);
				if (result != S_OK) {
					std::this_thread::yield();
				}
			} while (result != S_OK);
			if (q.type == TimestampType::FrameStart) {
				firstTick = tick;
			}

			const auto time = uint64_t((static_cast<double>(tick - firstTick) / static_cast<double>(disjointData.Frequency)) * 1'000'000'000);
			onTimestamp(q.snapshot, q.type, q.id, time);
		}
		clearQueries();
	}
}
