#include "halley/graphics/painter.h"

#include <cassert>

#include "halley/graphics/render_context.h"
#include "halley/graphics/render_target/render_target.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"
#include <cstring> // memmove
#include <gsl/assert>


#include "halley/api/video_api.h"
#include "halley/graphics/render_snapshot.h"
#include "halley/maths/bezier.h"
#include "halley/maths/polygon.h"
#include "halley/support/logger.h"
#include "halley/support/profiler.h"
#include "halley/utils/algorithm.h"
#include "halley/resources/resources.h"

using namespace Halley;

struct LineVertex {
	Vector4f colour;
	Vector4f dashing;
	Vector2f position;
	Vector2f normal;
	Vector2f width;
	char _padding[8];
};

Painter::Painter(VideoAPI& video, Resources& resources)
	: halleyGlobalMaterial(std::unique_ptr<Material>(new Material(resources.get<MaterialDefinition>("Halley/MaterialBase"), 0)))
	, resources(resources)
	, video(video)
	, solidLineMaterial(std::make_unique<Material>(resources.get<MaterialDefinition>("Halley/SolidLine")))
	, solidPolygonMaterial(std::make_unique<Material>(resources.get<MaterialDefinition>("Halley/SolidPolygon")))
	, blitMaterial(std::make_unique<Material>(resources.get<MaterialDefinition>("Halley/Blit")))
	, blitDepthMaterial(std::make_unique<Material>(resources.get<MaterialDefinition>("Halley/BlitDepth")))
	, objectDataBuffer(video.createShaderStorageBuffer())
{
}

Painter::~Painter()
{
}

void Painter::startRender()
{
	Material::resetBindCache();
	prevDrawCalls = nDrawCalls;
	prevTriangles = nTriangles;
	prevVertices = nVertices;
	nDrawCalls = nTriangles = nVertices = 0;
	frameStart = frameEnd = 0;

	refreshConstantBufferCache();
	resetPending();
	doStartRender();
}

void Painter::endRender()
{
	flush();
	
	ProfilerEvent event(ProfilerEventType::PainterEndRender);
	doEndRender();

	stopRecording();

	camera = Camera();
	viewPort = Rect4i(0, 0, 0, 0);
}

void Painter::flush()
{
	flushPending();
}

void Painter::resetState()
{
	curClip = Rect4i(0, 0, 1, 1);
	setClip();
	updateClip();
}

Rect4f Painter::getWorldViewAABB() const
{
	Vector2f size = Vector2f(viewPort.getSize()) / camera.getZoom();
	assert(camera.getZAngle().getRadians() == 0); // Camera rotation not accounted by following line
	auto camPos = camera.getPosition();
	return Rect4f(Vector2f(camPos.x, camPos.y) - size * 0.5f, size.x, size.y);
}

void Painter::clear(std::optional<Colour> colour, std::optional<float> depth, std::optional<uint8_t> stencil)
{
	if (recordingSnapshot) {
		const auto commandIdx = recordingSnapshot->getNumCommands();
		recordingSnapshot->clear(colour, depth, stencil);
		recordTimestamp(TimestampType::CommandStart, commandIdx);
		doClear(colour, depth, stencil);
		recordTimestamp(TimestampType::CommandEnd, commandIdx);
	} else {
		doClear(colour, depth, stencil);
	}
}

Painter::PainterVertexData Painter::addDrawData(const std::shared_ptr<const Material>& material, size_t numObjects, size_t numVertices, size_t numIndices, bool standardQuadsOnly)
{
	Expects(material != nullptr);
	Expects(numVertices > 0);
	Expects(numIndices >= numVertices);
	const auto& matDef = material->getDefinition();

	updateClip();

	// Check absolute max values
	constexpr auto maxVertices = static_cast<size_t>(std::numeric_limits<IndexType>::max()) + 1;
	const auto maxObjects = getMaxObjects(matDef);
	if (numVertices > maxVertices) {
		throw Exception("Too many vertices in draw call: " + toString(numVertices) + ", maximum is " + toString(maxVertices), HalleyExceptions::Graphics);
	}
	if (numObjects > maxObjects) {
		throw Exception("Too many objects in draw call: " + toString(numObjects) + ", maximum is " + toString(maxObjects), HalleyExceptions::Graphics);
	}

	// Flush existing if it won't fit
	if (verticesPending + numVertices > maxVertices || objectsPending + numObjects > maxObjects) {
		flushPending();
	}

	// Calculate space needed
	PainterVertexData result;
	result.vertexSize = matDef.getVertexSize();
	result.vertexStride = matDef.getVertexStride();
	result.vertexDataSize = numVertices * result.vertexStride;
	result.objectSize = matDef.getObjectSize();
	result.objectStride = matDef.getObjectStride();
	result.objectDataSize = numObjects * result.objectStride;

	// Start new draw call if needed
	startDrawCall(material);

	// Allocate space
	makeSpaceForPendingVertices(result.vertexDataSize);
	makeSpaceForPendingObjects(result.objectDataSize);
	makeSpaceForPendingIndices(numIndices);
	result.dstVertex = vertexBuffer.data() + vertexBytesPending;
	result.dstObject = objectBuffer.data() + objectBytesPending;
	result.dstIndex = indexBuffer.data() + indicesPending;
	result.firstIndex = static_cast<IndexType>(verticesPending);
	result.firstObjectIndex = static_cast<int>(objectsPending);

	indicesPending += numIndices;
	verticesPending += numVertices;
	objectsPending += numObjects;
	vertexBytesPending += result.vertexDataSize;
	objectBytesPending += result.objectDataSize;
	allIndicesAreQuads &= standardQuadsOnly;

	pendingDebugGroupStack = curDebugGroupStack;

	return result;
}

void Painter::draw(const std::shared_ptr<const Material>& material, size_t numVertices, const void* vertexData, gsl::span<const IndexType> indices, PrimitiveType primitiveType)
{
	Expects(primitiveType == PrimitiveType::Triangle);
	Expects(indices.size() % 3 == 0);

	const auto result = addDrawData(material, 1, numVertices, indices.size(), false);

	memcpy(result.dstVertex, vertexData, result.vertexDataSize);

	for (size_t i = 0; i < size_t(indices.size()); ++i) {
		result.dstIndex[i] = indices[i] + result.firstIndex;
	}
}

void Painter::drawQuads(const std::shared_ptr<const Material>& material, size_t numVertices, const void* vertexData)
{
	Expects(numVertices % 4 == 0);
	Expects(vertexData != nullptr);

	const auto result = addDrawData(material, 1, numVertices, numVertices * 3 / 2, true);

	memcpy(result.dstVertex, vertexData, result.vertexDataSize);
	generateQuadIndices(result.firstIndex, numVertices / 4, result.dstIndex);
}

void Painter::drawSprites(const std::shared_ptr<const Material>& material, size_t totalNumSprites, const void* objectData)
{
	Expects(objectData != nullptr);

	constexpr size_t verticesPerSprite = 4;
	const size_t maxSpritesPerCall = std::min((static_cast<size_t>(std::numeric_limits<IndexType>::max()) + 1) / verticesPerSprite, getMaxObjects(material->getDefinition()));
	size_t numSpritesLeft = totalNumSprites;
	size_t objectOffset = 0;

	while (numSpritesLeft > 0) {
		const size_t numSprites = std::min(numSpritesLeft, maxSpritesPerCall);
		const size_t numVertices = verticesPerSprite * numSprites;

		const auto result = addDrawData(material, numSprites, numVertices, numSprites * 6, true);
		const char* const src = static_cast<const char*>(objectData) + objectOffset;

		if (result.objectDataSize > 0) {
			memcpy(result.dstObject, src, result.objectDataSize);
		}

		for (size_t i = 0; i < numSprites; i++) {
			const auto objectIdx = static_cast<int>(i) + result.firstObjectIndex;

			for (size_t j = 0; j < verticesPerSprite; j++) {
				const size_t dstOffset = (i * verticesPerSprite + j) * result.vertexStride;

				constexpr static Vector2f vertPosList[] = { Vector2f(0, 0), Vector2f(1, 0), Vector2f(1, 1), Vector2f(0, 1)};
				VertexData vertData;
				vertData.pos = Vector4f(vertPosList[j], vertPosList[j]);
				vertData.idx = objectIdx;
				memcpy(result.dstVertex + dstOffset, &vertData, sizeof(vertData));
			}
		}

		generateQuadIndices(result.firstIndex, numSprites, result.dstIndex);

		numSpritesLeft -= numSprites;
		objectOffset += numSprites * material->getDefinition().getObjectStride();
	}
}

void Painter::drawSlicedSprite(const std::shared_ptr<const Material>& material, Vector2f scale, Vector4f slices, const void* objectData)
{
	Expects(objectData != nullptr);
	if (scale.x < 0.00001f || scale.y < 0.00001f) {
		//throw Exception("Scale is zero for material with texture " + material->getTexture(0)->getAssetId());
		return;
	}
	//Expects(scale.x > 0.0001f);
	//Expects(scale.y > 0.0001f);

	//         a        c
	//   00 -- 01 ----- 02 -- 03
	//   |     |        |     |
	// b 04 -- 05 ----- 06 -- 07
	//   |     |        |     |
	//   |     |        |     |
	// d 08 -- 09 ----- 10 -- 11
	//   |     |        |     |
	//   12 -- 13 ----- 14 -- 15

	constexpr size_t numVertices = 16;
	constexpr size_t numIndices = 9 * 6; // 9 quads, 6 indices per quad

	const auto result = addDrawData(material, 1, numVertices, numIndices, false);

	if (result.objectDataSize > 0) {
		memcpy(result.dstObject, objectData, result.objectDataSize);
	}

	// Vertices
	std::array<Vector2f, 4> pos = {{ Vector2f(0, 0), Vector2f(slices.x / scale.x, slices.y / scale.y), Vector2f(1 - slices.z / scale.x, 1 - slices.w / scale.y), Vector2f(1, 1) }};
	std::array<Vector2f, 4> tex = {{ Vector2f(0, 0), Vector2f(slices.x, slices.y), Vector2f(1 - slices.z, 1 - slices.w), Vector2f(1, 1) }};
	for (size_t i = 0; i < numVertices; i++) {
		const size_t ix = i & 3;
		const size_t iy = i >> 2;
		const size_t dstOffset = i * result.vertexStride;

		VertexData vert;
		vert.pos = Vector4f(pos[ix].x, pos[iy].y, tex[ix].x, tex[iy].y);
		vert.idx = result.firstObjectIndex;
		memcpy(result.dstVertex + dstOffset, &vert, result.vertexSize);
	}

	// Indices
	IndexType* dstIndex = result.dstIndex;
	for (size_t y = 0; y < 3; y++) {
		for (size_t x = 0; x < 3; x++) {
			generateQuadIndicesOffset(static_cast<IndexType>(result.firstIndex + x + (y * 4)), 4, dstIndex);
			dstIndex += 6;
		}
	}
}

void Painter::drawLine(gsl::span<const Vector2f> points, float width, Colour4f colour, bool loop, std::shared_ptr<const Material> material, LineParameters params)
{
	if (!material) {
		material = getSolidLineMaterial();
	}

	// Need at least two points to draw a line
	if (points.size() < 2) {
		return;
	}

	constexpr float normalPos[] = { -1, 1, 1, -1 };
	constexpr size_t pointIdxOffset[] = { 0, 0, 1, 1 };

	const size_t nPoints = points.size();
	const size_t nSegments = (loop ? nPoints : (nPoints - 1));
	Vector<LineVertex> vertices(nSegments * 4);

	auto segmentNormal = [&] (size_t i) -> std::optional<Vector2f>
	{
		if (!loop && i >= nSegments) {
			return {};
		} else {
			return (points[(i + 1) % nPoints] - points[i % nPoints]).normalized().orthoLeft();
		}
	};

	auto makeNormal = [] (Vector2f a, std::optional<Vector2f> maybeB) -> Vector2f
	{
		// Enabling this makes it looks nicer, but also introduces a lot of edge cases with acute angles that are very hard to deal with, so only enable for angles >= 90 degrees
		if (maybeB && maybeB.value().dot(a) >= -0.001f) {
			const auto b = maybeB.value();
			const auto c = (a + b).normalized();
			const auto cosHalfAlpha = c.dot(a);
			return c * (1.0f / cosHalfAlpha);
		} else {
			return a;
		}
	};

	std::optional<Vector2f> prevNormal = loop ? segmentNormal(nSegments - 1) : std::optional<Vector2f>();
	Vector2f normal = segmentNormal(0).value();

	const float zoom = getCurrentCamera().getZoom();
	const float pixelAlignOffset = params.pixelAlign ? std::fmod(width * zoom, 2.0f) / (2.0f * zoom) : 0.0f;

	float curLen = 0;
	for (size_t i = 0; i < nSegments; ++i) {
		std::optional<Vector2f> nextNormal = segmentNormal(i + 1);

		const Vector2f v0n = makeNormal(normal, prevNormal);
		const Vector2f v1n = makeNormal(normal, nextNormal);

		const float segmentLength = (points[(i + 1) % nPoints] - points[i]).length();
		const float nextCurLen = curLen + segmentLength;
		std::array<float, 2> curLens = { curLen, nextCurLen };
		curLen = nextCurLen;

		for (size_t j = 0; j < 4; ++j) {
			const size_t idx = i * 4 + j;
			auto& v = vertices[idx];
			v.colour = colour.toVector4();
			v.position = points[(i + pointIdxOffset[j]) % nPoints] + Vector2f(pixelAlignOffset, pixelAlignOffset);
			v.normal = j <= 1 ? v0n : v1n;
			v.width.x = width;
			v.width.y = normalPos[j];
			v.dashing = Vector4f(curLens[pointIdxOffset[j]], params.onLength, params.offLength, 0);
		}

		prevNormal = normal;
		if (nextNormal) {
			normal = nextNormal.value();
		}
	}

	drawQuads(material, vertices.size(), vertices.data());
}

void Painter::drawLine(const LineSegment& line, float width, Colour4f colour, bool loop, std::shared_ptr<const Material> material, LineParameters params)
{
	drawLine(gsl::span<const Vector2f>(&line.a, 2), width, colour, loop, std::move(material), params);
}

void Painter::drawLine(const BezierQuadratic& bezier, float width, Colour4f colour, std::shared_ptr<const Material> material, LineParameters params)
{
	auto points = bezier.toLineSegments();
	drawLine(points, width, colour, false, material, params);
}

void Painter::drawLine(const BezierCubic& bezier, float width, Colour4f colour, std::shared_ptr<const Material> material, LineParameters params)
{
	auto points = bezier.toLineSegments();
	drawLine(points, width, colour, false, material, params);
}

void Painter::drawArrow(Vector2f from, Vector2f to, float headSize, float width, Colour4f colour, Vector2f anisotropy, std::shared_ptr<const Material> material)
{
	const auto fwd = (to - from).normalized();
	const auto right = (fwd * anisotropy).orthoRight() / anisotropy;

	const auto p0 = to;
	const auto p3 = from;
	const auto p1 = p0 - (headSize * fwd) + (headSize * 0.5f * right);
	const auto p2 = p0 - (headSize * fwd) - (headSize * 0.5f * right);

	drawLine(Vector<Vector2f>{{ p3, p0 }}, width, colour, false, material);
	drawLine(Vector<Vector2f>{{ p1, to }}, width, colour, false, material);
	drawLine(Vector<Vector2f>{{ p2, to }}, width, colour, false, material);
}

static size_t getSegmentsForArc(float radius, float arcLen)
{
	return clamp(size_t(arcLen / float(pi() * 2) * 50.0f), size_t(4), size_t(256));
}

void Painter::drawCircle(Vector2f centre, float radius, float width, Colour4f colour, std::shared_ptr<const Material> material, LineParameters params)
{
	const size_t n = getSegmentsForArc(radius, 2 * float(pi()));
	Vector<Vector2f> points;
	for (size_t i = 0; i < n; ++i) {
		points.push_back(centre + Vector2f(radius, 0).rotate(Angle1f::fromRadians(i * 2.0f * float(pi()) / n)));
	}
	drawLine(points, width, colour, true, std::move(material), params);
}

void Painter::drawCircle(Circle circle, float width, Colour4f colour, std::shared_ptr<const Material> material, LineParameters params)
{
	drawCircle(circle.getCentre(), circle.getRadius(), width, colour, std::move(material), params);
}

void Painter::drawCircleArc(Vector2f centre, float radius, float width, Angle1f from, Angle1f to, Colour4f colour, std::shared_ptr<const Material> material, LineParameters params)
{
	const float arcLen = (to - from).getRadians() + (from.turnSide(to) > 0 ? 0.0f : 0 * float(pi()));
	const size_t n = getSegmentsForArc(radius, arcLen);
	Vector<Vector2f> points;
	for (size_t i = 0; i < n; ++i) {
		points.push_back(centre + Vector2f(radius, 0).rotate(from + Angle1f::fromRadians(i * arcLen / (n - 1))));
	}
	drawLine(points, width, colour, false, std::move(material), params);
}

void Painter::drawCircleArc(Circle circle, float width, Angle1f from, Angle1f to, Colour4f colour, std::shared_ptr<const Material> material, LineParameters params)
{
	drawCircleArc(circle.getCentre(), circle.getRadius(), width, from, to, colour, std::move(material), params);
}

void Painter::drawEllipse(Vector2f centre, Vector2f radius, float width, Colour4f colour, std::shared_ptr<const Material> material, LineParameters params)
{
	const size_t n = getSegmentsForArc(std::max(radius.x, radius.y), 2 * float(pi()));
	Vector<Vector2f> points;
	for (size_t i = 0; i < n; ++i) {
		points.push_back(centre + Vector2f(1.0f, 0).rotate(Angle1f::fromRadians(i * 2.0f * float(pi()) / n)) * radius);
	}
	drawLine(points, width, colour, true, std::move(material), params);
}

void Painter::drawRect(Rect4f rect, float width, Colour4f colour, std::shared_ptr<const Material> material, LineParameters params)
{
	Vector<Vector2f> points;
	points.push_back(rect.getTopLeft());
	points.push_back(rect.getTopRight());
	points.push_back(rect.getBottomRight());
	points.push_back(rect.getBottomLeft());
	drawLine(points, width, colour, true, std::move(material), params);
}

void Painter::drawPolygon(const Polygon& polygon, Colour4f colour, std::shared_ptr<const Material> material)
{
	if (!polygon.isValid()) {
		return;
	}
	
	if (!material) {
		material = getSolidPolygonMaterial();
	}
	
	if (!polygon.isConvex()) {
		for (const auto& p: polygon.splitIntoConvex()) {
			drawPolygon(p, colour, material);
		}
		return;
	}

	auto col = Vector4f(colour.r, colour.g, colour.b, colour.a);
	
	const auto& vs = polygon.getVertices();
	const auto n = vs.size();
	Vector<LineVertex> vertices(n);
	for (size_t i = 0; i < n; ++i) {
		vertices[i].position = vs[i];
		vertices[i].colour = col;
		vertices[i].normal = Vector2f();
		vertices[i].width = Vector2f();
	}
	Vector<IndexType> indices((n - 2) * 3);
	for (size_t i = 0; i < n - 2; ++i) {
		indices.push_back(0);
		indices.push_back(static_cast<IndexType>(i) + 1);
		indices.push_back(static_cast<IndexType>(i) + 2);
	}

	draw(material, vertices.size(), vertices.data(), indices, PrimitiveType::Triangle);
}

void Painter::blitTexture(const std::shared_ptr<const Texture>& texture, TargetBufferType blitType)
{
	struct BlitVertex {
		Vector4f p;
		Vector4f t;
	};
	std::array<BlitVertex, 4> vs;
	vs[0] = BlitVertex{ Vector4f(-1, -1, 0, 1), Vector4f(0, 1, 0, 0) };
	vs[1] = BlitVertex{ Vector4f(1, -1, 0, 1),  Vector4f(1, 1, 0, 0) };
	vs[2] = BlitVertex{ Vector4f(1, 1, 0, 1),   Vector4f(1, 0, 0, 0) };
	vs[3] = BlitVertex{ Vector4f(-1, 1, 0, 1),  Vector4f(0, 0, 0, 0) };

	std::array<uint16_t, 6> indices = { 0, 1, 3, 1, 2, 3 };

	const auto material = blitType == TargetBufferType::Depth ? blitDepthMaterial : blitMaterial;
	material->set(0, texture);
	draw(material, 4, vs.data(), indices, PrimitiveType::Triangle);
	flushPending();
	material->set(0, std::shared_ptr<const Texture>{});
}

void Painter::setLogging(bool logging)
{
	this->logging = logging;
}

void Painter::pushDebugGroup(const String& id)
{
	flush();
	curDebugGroupStack.push_back(id);
}

void Painter::popDebugGroup()
{
	flush();
	curDebugGroupStack.pop_back();
}

void Painter::startRecording(RenderSnapshot* snapshot)
{
	flush();
	stopRecording();
	recordingSnapshot = snapshot;
	if (recordingSnapshot) {
		recordingSnapshot->start();
	}

	recordingPerformance = startPerformanceMeasurement();
	if (recordingPerformance) {
		recordTimestamp(TimestampType::FrameStart, 0);
		frameStartCPUTime = std::chrono::steady_clock::now();
	}
}

void Painter::stopRecording()
{
	if (recordingPerformance) {
		flush();

		recordTimestamp(TimestampType::FrameEnd, 0);
		endPerformanceMeasurement();

		if (recordingSnapshot) {
			recordingSnapshot->end();
			recordingSnapshot = nullptr;
		}
		recordingPerformance = false;
	}
}

bool Painter::startPerformanceMeasurement()
{
	return false;
}

void Painter::endPerformanceMeasurement()
{
}

void Painter::recordTimestamp(TimestampType type, size_t id)
{
	if (recordingPerformance) {
		if (recordingSnapshot) {
			recordingSnapshot->addPendingTimestamp();
		}

		doRecordTimestamp(type, id, recordingSnapshot);
	}
}

void Painter::doRecordTimestamp(TimestampType type, size_t id, ITimestampRecorder* snapshot)
{
}

void Painter::onTimestamp(ITimestampRecorder* snapshot, TimestampType type, size_t idx, uint64_t value)
{
	if (type == TimestampType::FrameStart) {
		frameStart = value;
	} else if (type == TimestampType::FrameEnd) {
		frameEnd = value;

		auto& profiler = ProfilerCapture::get();
		const auto profilerEventId = profiler.recordEventStart(ProfilerEventType::GPU, "", frameStartCPUTime);
		profiler.recordEventEnd(profilerEventId, frameStartCPUTime + std::chrono::nanoseconds(frameEnd - frameStart));
	}

	if (snapshot) {
		snapshot->onTimestamp(type, idx, value);
	}
}

void Painter::makeSpaceForPendingVertices(size_t numBytes)
{
	size_t requiredSize = vertexBytesPending + numBytes;
	if (vertexBuffer.size() < requiredSize) {
		vertexBuffer.resize(requiredSize * 2);
	}
}

void Painter::makeSpaceForPendingObjects(size_t numBytes)
{
	size_t requiredSize = objectBytesPending + numBytes;
	if (objectBuffer.size() < requiredSize) {
		objectBuffer.resize(requiredSize * 2);
	}
}

void Painter::makeSpaceForPendingIndices(size_t numIndices)
{
	size_t requiredSize = indicesPending + numIndices;
	if (indexBuffer.size() < requiredSize) {
		indexBuffer.resize(requiredSize * 2);
	}
}

void Painter::bind(RenderContext& context)
{
	if (recordingSnapshot) {
		recordingSnapshot->bind(context);
	}

	doBind(context.getCamera(), context.getDefaultRenderTarget());
}

void Painter::unbind(RenderContext& context)
{
	flush();

	if (recordingSnapshot) {
		recordingSnapshot->unbind(context);
	}

	doUnbind();
}

void Painter::doBind(const Camera& cam, RenderTarget& renderTarget)
{
	// Setup camera
	camera = cam;
	
	// Set render target
	activeRenderTarget = &renderTarget;
	if (!activeRenderTarget) {
		throw Exception("No active render target", HalleyExceptions::Core);
	}
	camera.activeRenderTarget = activeRenderTarget;
	activeRenderTarget->onBind(*this);

	// Set viewport
	viewPort = camera.getActiveViewPort();
	setViewPort(getRectangleForActiveRenderTarget(viewPort));
	setClip();

	// Update projection
	updateProjection();
}

void Painter::doUnbind()
{
	if (activeRenderTarget) {
		activeRenderTarget->onUnbind(*this);
		activeRenderTarget = nullptr;
		camera.activeRenderTarget = nullptr;
	}
}

void Painter::setRelativeClip(Rect4f rect)
{
	std::array<Vector2f, 4> ps = {{ rect.getTopLeft(), rect.getTopRight(), rect.getBottomLeft(), rect.getBottomRight() }};
	float x0 = -std::numeric_limits<float>::infinity();
	float x1 = std::numeric_limits<float>::infinity();
	float y0 = -std::numeric_limits<float>::infinity();
	float y1 = std::numeric_limits<float>::infinity();
	for (auto& p: ps) {
		auto point = camera.worldToScreen(p, Rect4f(viewPort));
		x0 = std::max(x0, point.x);
		x1 = std::min(x1, point.x);
		y0 = std::max(y0, point.y);
		y1 = std::min(y1, point.y);
	}
	setClip(Rect4i(Vector2i(Vector2f(x0, y0).floor()), Vector2i(Vector2f(x1, y1).ceil())));
}

void Painter::setClip(std::optional<Rect4i> rect)
{
	pendingClip = rect;
}

Rect4i Painter::getRectangleForActiveRenderTarget(Rect4i r)
{
	Expects(activeRenderTarget);
	int h = activeRenderTarget->getViewPort().getHeight();
	if (activeRenderTarget->getViewportFlipVertical()) {
		int y = h - r.getBottom();
		return Rect4i(Vector2i(r.getLeft(), y), r.getWidth(), r.getHeight());
	} else {
		return r;
	}
}

std::shared_ptr<const Material> Painter::getSolidLineMaterial()
{
	return solidLineMaterial;
}

std::shared_ptr<const Material> Painter::getSolidPolygonMaterial()
{
	return solidPolygonMaterial;
}

MaterialConstantBuffer& Painter::getConstantBuffer(const MaterialDataBlock& dataBlock)
{
	const uint64_t hash = dataBlock.getHash();
	const auto iter = constantBuffers.find(hash);
	if (iter == constantBuffers.end()) {
		auto buffer = std::shared_ptr<MaterialConstantBuffer>(video.createConstantBuffer());
		buffer->update(dataBlock.getData());
		constantBuffers[hash] = ConstantBufferEntry{ buffer, 0 };
		return *buffer;
	} else {
		return *iter->second.buffer;
	}
}

void Painter::refreshConstantBufferCache()
{
	for (auto& [k, v]: constantBuffers) {
		++v.age;
	}
	std_ex::erase_if_value(constantBuffers, [] (const ConstantBufferEntry& e) { return e.age >= 10; });
}

size_t Painter::getMaxObjects(const MaterialDefinition& material) const
{
	const size_t perObject = material.getObjectStride();
	if (perObject == 0) {
		return std::numeric_limits<size_t>::max();
	}
	const size_t maxSize = 16 * 1024; // TODO
	return (maxSize + (perObject - 1)) / perObject;
}

void Painter::startDrawCall(const std::shared_ptr<const Material>& material)
{
	constexpr bool enableDynamicBatching = true;

	if (material != materialPending || pendingDebugGroupStack != curDebugGroupStack) {
		if (!enableDynamicBatching || (materialPending != std::shared_ptr<const Material>() && !(*material == *materialPending))) {
			flushPending();
		}
		materialPending = material;
	}
}

void Painter::flushPending()
{
	if (verticesPending > 0) {
		const auto vertexSpan = gsl::span<char>(vertexBuffer.data(), verticesPending * materialPending->getDefinition().getVertexStride());
		const auto objectSpan = gsl::span<char>(objectBuffer.data(), objectsPending * materialPending->getDefinition().getObjectStride());
		const auto indexSpan = gsl::span<const IndexType>(indexBuffer.data(), indicesPending);
		executeDrawPrimitives(*materialPending, objectsPending, verticesPending, objectSpan, vertexSpan, indexSpan, PrimitiveType::Triangle, allIndicesAreQuads);
	}

	resetPending();
}

void Painter::resetPending()
{
	vertexBytesPending = 0;
	verticesPending = 0;
	objectBytesPending = 0;
	objectsPending = 0;
	indicesPending = 0;
	allIndicesAreQuads = true;
	if (materialPending) {
		Material::resetBindCache();
		materialPending.reset();
	}
	pendingDebugGroupStack = curDebugGroupStack;
}

void Painter::executeDrawPrimitives(const Material& material, size_t numObjects, size_t numVertices, gsl::span<const char> objectData, gsl::span<const char> vertexData, gsl::span<const IndexType> indices, PrimitiveType primitiveType, bool allIndicesAreQuads)
{
	Expects(primitiveType == PrimitiveType::Triangle);

	ProfilerEvent event(ProfilerEventType::PainterDrawCall);

	size_t commandIdx = 0;
	if (recordingSnapshot) {
		commandIdx = recordingSnapshot->getNumCommands();
		recordingSnapshot->draw(material, numObjects, numVertices, objectData, vertexData, indices, primitiveType, allIndicesAreQuads);
		recordTimestamp(TimestampType::CommandStart, commandIdx);
	}

	startDrawCall();

	// Load vertices
	setVertices(material.getDefinition(), numVertices, vertexData.data(), indices.size(), indices.data(), allIndicesAreQuads);
	
	// Load object data
	if (!objectData.empty()) {
		objectDataBuffer->update(numObjects, objectData.size_bytes() / numObjects, gsl::as_bytes(objectData));
		objectDataBuffer->bind(ShaderType::Vertex, 0);
	}

	// Load material uniforms
	setMaterialData(material);

	// Go through each pass
	for (int i = 0; i < material.getDefinition().getNumPasses(); i++) {
		if (material.isPassEnabled(i)) {
			// Bind pass
			material.bind(i, *this);
			if (recordingSnapshot) {
				recordTimestamp(TimestampType::CommandSetupDone, commandIdx);
			}

			// Draw
			drawTriangles(indices.size());

			// Log stats
			if (logging) {
				nDrawCalls++;
				nTriangles += indices.size() / 3;
				nVertices += numVertices;
			}
		}
	}

	endDrawCall();

	if (recordingSnapshot) {
		recordTimestamp(TimestampType::CommandEnd, commandIdx);
	}
}

IndexType* Painter::getStandardQuadIndices(size_t numQuads)
{
	size_t sz = numQuads * 6;
	size_t oldSize = stdQuadIndexCache.size();

	if (oldSize < sz) {
		stdQuadIndexCache.resize(sz);
		IndexType pos = static_cast<IndexType>(oldSize * 2 / 3);
		for (size_t i = oldSize; i < sz; i += 6) {
			// A-----B
			// |     |
			// D-----C
			// ABC
			stdQuadIndexCache[i] = pos;
			stdQuadIndexCache[i + 1] = pos + 1;
			stdQuadIndexCache[i + 2] = pos + 2;
			// CDA
			stdQuadIndexCache[i + 3] = pos + 2;
			stdQuadIndexCache[i + 4] = pos + 3;
			stdQuadIndexCache[i + 5] = pos;
			pos += 4;
		}
	}

	return stdQuadIndexCache.data();
}

void Painter::generateQuadIndices(IndexType pos, size_t numQuads, IndexType* target)
{
	size_t numIndices = numQuads * 6;
	for (size_t i = 0; i < numIndices; i += 6) {
		// A-----B
		// |     |
		// D-----C
		// ABC
		target[i] = pos;
		target[i + 1] = pos + 1;
		target[i + 2] = pos + 2;
		// CDA
		target[i + 3] = pos + 2;
		target[i + 4] = pos + 3;
		target[i + 5] = pos;
		pos += 4;
	}
}

RenderTarget& Painter::getActiveRenderTarget()
{
	Expects(activeRenderTarget);
	return *activeRenderTarget;
}

const RenderTarget* Painter::tryGetActiveRenderTarget() const
{
	return activeRenderTarget;
}

const Vector<String>& Painter::getPendingDebugGroupStack() const
{
	return pendingDebugGroupStack;
}

void Painter::generateQuadIndicesOffset(IndexType pos, IndexType lineStride, IndexType* target)
{
	// A-----B
	// |     |
	// C-----D
	// ABD
	target[0] = pos;
	target[1] = pos + 1;
	target[2] = pos + lineStride + 1;
	// DCA
	target[3] = pos + lineStride + 1;
	target[4] = pos + lineStride;
	target[5] = pos;
}

void Painter::updateProjection()
{
	ProfilerEvent event(ProfilerEventType::PainterUpdateProjection);
	
	camera.updateProjection(activeRenderTarget->getProjectionFlipVertical());
	projection = camera.getProjection();

	auto viewPortSize = (Vector2f(camera.getActiveViewPort().getSize()) / Vector2f(2, 2)).ceil() * Vector2f(2, 2);

	const auto oldHash = halleyGlobalMaterial->getFullHash();
	halleyGlobalMaterial->set("u_mvp", projection);
	halleyGlobalMaterial->set("u_viewPortSize", viewPortSize);
	onUpdateProjection(*halleyGlobalMaterial, oldHash != halleyGlobalMaterial->getFullHash());
}

void Painter::updateClip()
{
	Rect4i finalRect = viewPort;
	if (pendingClip) {
		finalRect = (pendingClip.value() + viewPort.getTopLeft()).intersection(viewPort);
	}
	const Rect4i targetClip = getRectangleForActiveRenderTarget(finalRect);
	const bool enableClip = finalRect != activeRenderTarget->getViewPort();
	const std::optional<Rect4i> dstClip = enableClip ? targetClip : std::optional<Rect4i>();

	if (curClip != dstClip) {
		curClip = dstClip;

		flushPending();
		setClip(targetClip, enableClip);
		if (recordingSnapshot) {
			recordingSnapshot->setClip(targetClip, enableClip);
		}
	}
}
