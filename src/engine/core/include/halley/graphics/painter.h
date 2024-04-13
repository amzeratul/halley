#pragma once
#include "camera.h"
#include "blend.h"
#include "halley/maths/colour.h"
#include "graphics_enums.h"
#include <condition_variable>
#include <halley/maths/vector4.h>


#include "render_snapshot.h"
#include "texture.h"
#include "halley/data_structures/hash_map.h"
#include "halley/maths/circle.h"
#include "halley/time/halleytime.h"

namespace Halley
{
	class RenderSnapshot;
	class LineSegment;
	class VideoAPI;
	class MaterialDataBlock;
	class MaterialConstantBuffer;
	class BezierCubic;
	class BezierQuadratic;
	class Polygon;
	class Resources;
	class Shader;
	class Material;
	class MaterialDefinition;
	class Camera;
	class RenderContext;
	class Core;

	class Painter
	{
		friend class RenderContext;
		friend class Core;
		friend class Material;
		friend class RenderSnapshot;

		struct PainterVertexData
		{
			char* dstVertex;
			char* dstObject;
			IndexType* dstIndex;
			size_t vertexSize;
			size_t vertexStride;
			size_t vertexDataSize;
			size_t objectSize;
			size_t objectStride;
			size_t objectDataSize;
			IndexType firstIndex;
			int firstObjectIndex;
		};

	public:
		struct LineParameters {
			LineParameters(bool pixelAlign = true, float onLength = 10.0f, float offLength = 0.0f) 
				: pixelAlign(pixelAlign)
				, onLength(onLength)
				, offLength(offLength)
			{ }

			bool pixelAlign = true;
			float onLength = 10.0f;
			float offLength = 0.0f;
		};

		struct VertexData {
			Vector4f pos;
			int idx;
		};

		Painter(VideoAPI& video, Resources& resources);
		virtual ~Painter();

		void flush();
		virtual void resetState();

		Rect4i getViewPort() const { return viewPort; }
		const Camera& getCurrentCamera() const { return camera; }
		Rect4f getWorldViewAABB() const;

		void clear(std::optional<Colour> colour, std::optional<float> depth = 1.0f, std::optional<uint8_t> stencil = 0);

		void setRelativeClip(Rect4f rect);
		void setClip(std::optional<Rect4i> rect = std::nullopt);

		// Draws primitives
		void draw(const std::shared_ptr<const Material>& material, size_t numVertices, const void* vertexData, gsl::span<const IndexType> indices, PrimitiveType primitiveType = PrimitiveType::Triangle);

		// Draws quads
		void drawQuads(const std::shared_ptr<const Material>& material, size_t numVertices, const void* vertexData);

		// Draw sprites takes a single vertex per sprite, duplicates the data across multiple vertices, and draws
		// vertPosOffset is the offset, in bytes, from the start of each vertex's data, to a Vector2f which will be filled with the vertex's position in 0-1 space.
		void drawSprites(const std::shared_ptr<const Material>& material, size_t numSprites, const void* objectData);

		// Draw one sliced sprite. Slices -> x = left, y = top, z = right, w = bottom, in [0..1] space relative to the texture
		void drawSlicedSprite(const std::shared_ptr<const Material>& material, Vector2f scale, Vector4f slices, const void* objectData);

		// Draws a line across all points (if no material is specified, use standard one)
		void drawLine(gsl::span<const Vector2f> points, float width, Colour4f colour, bool loop = false, std::shared_ptr<const Material> material = {}, LineParameters params = {});
		void drawLine(const LineSegment& line, float width, Colour4f colour, bool loop = false, std::shared_ptr<const Material> material = {}, LineParameters params = {});
		void drawLine(const BezierQuadratic& bezier, float width, Colour4f colour, std::shared_ptr<const Material> material = {}, LineParameters params = {});
		void drawLine(const BezierCubic& bezier, float width, Colour4f colour, std::shared_ptr<const Material> material = {}, LineParameters params = {});

		// Draw arrow
		void drawArrow(Vector2f from, Vector2f to, float headSize, float width, Colour4f colour, Vector2f anisotropy = Vector2f(1, 1), std::shared_ptr<const Material> material = {});

		// Circle drawing
		void drawCircle(Vector2f centre, float radius, float width, Colour4f colour, std::shared_ptr<const Material> material = {}, LineParameters params = {});
		void drawCircle(Circle circle, float width, Colour4f colour, std::shared_ptr<const Material> material = {}, LineParameters params = {});
		void drawCircleArc(Vector2f centre, float radius, float width, Angle1f from, Angle1f to, Colour4f colour, std::shared_ptr<const Material> material = {}, LineParameters params = {});
		void drawCircleArc(Circle circle, float width, Angle1f from, Angle1f to, Colour4f colour, std::shared_ptr<const Material> material = {}, LineParameters params = {});
		void drawEllipse(Vector2f centre, Vector2f radius, float width, Colour4f colour, std::shared_ptr<const Material> material = {}, LineParameters params = {});

		// Rect drawing
		void drawRect(Rect4f rect, float width, Colour4f colour, std::shared_ptr<const Material> material = {}, LineParameters params = {});

		// Polygon drawing
		void drawPolygon(const Polygon& polygon, Colour4f colour, std::shared_ptr<const Material> material = {});

		// Blit a texture over
		void blitTexture(const std::shared_ptr<const Texture>& texture, TargetBufferType blitType = TargetBufferType::Colour);

		size_t getNumDrawCalls() const { return nDrawCalls; }
		size_t getNumVertices() const { return nVertices; }
		size_t getNumTriangles() const { return nTriangles; }

		size_t getPrevDrawCalls() const { return prevDrawCalls; }
		size_t getPrevVertices() const { return prevVertices; }
		size_t getPrevTriangles() const { return prevTriangles; }

		void setLogging(bool logging);

		void pushDebugGroup(const String& id);
		void popDebugGroup();

		void startRecording(RenderSnapshot* snapshot);
		void stopRecording();

	protected:
		virtual void doStartRender() = 0;
		virtual void doEndRender() = 0;
		virtual void setVertices(const MaterialDefinition& material, size_t numVertices, const void* vertexData, size_t numIndices, const IndexType* indices, bool standardQuadsOnly) = 0;
		virtual void drawTriangles(size_t numIndices) = 0;

		virtual void doClear(std::optional<Colour> colour, std::optional<float> depth = 1.0f, std::optional<uint8_t> stencil = 0) = 0;

		virtual void setMaterialPass(const Material& material, int pass) = 0;
		virtual void setMaterialData(const Material& material) = 0;

		virtual void setViewPort(Rect4i rect) = 0;
		virtual void setClip(Rect4i clip, bool enable) = 0;

		virtual void onUpdateProjection(Material& material, bool hashChanged) = 0;

		virtual bool startPerformanceMeasurement();
		virtual void endPerformanceMeasurement();
		void recordTimestamp(TimestampType type, size_t id);
		virtual void doRecordTimestamp(TimestampType type, size_t id, ITimestampRecorder* snapshot);
		void onTimestamp(ITimestampRecorder* snapshot, TimestampType type, size_t idx, uint64_t value);

		virtual void startDrawCall() {}
		virtual void endDrawCall() {}
		virtual void onFinishRender() {}

		void generateQuadIndices(IndexType firstVertex, size_t numQuads, IndexType* target);
		RenderTarget& getActiveRenderTarget();
		const RenderTarget* tryGetActiveRenderTarget() const;

		const Vector<String>& getPendingDebugGroupStack() const;

		MaterialConstantBuffer& getConstantBuffer(const MaterialDataBlock& dataBlock);

		std::unique_ptr<Material> halleyGlobalMaterial;
		std::unique_ptr<Material> objectAttributeMaterial;

	private:
		class ConstantBufferEntry {
		public:
			std::shared_ptr<MaterialConstantBuffer> buffer;
			int age = 0;
		};
		
		Resources& resources;
		VideoAPI& video;
		RenderContext* activeContext = nullptr;
		RenderTarget* activeRenderTarget = nullptr;
		Matrix4f projection;
		Rect4i viewPort;
		Camera camera;

		size_t verticesPending = 0;
		size_t indicesPending = 0;
		size_t objectsPending = 0;
		size_t vertexBytesPending = 0;
		size_t objectBytesPending = 0;
		bool allIndicesAreQuads = true;
		Vector<char> vertexBuffer;
		Vector<char> objectBuffer;
		Vector<IndexType> indexBuffer;
		std::shared_ptr<const Material> materialPending;
		std::shared_ptr<const Material> solidLineMaterial;
		std::shared_ptr<const Material> solidPolygonMaterial;
		std::shared_ptr<Material> blitMaterial;
		std::shared_ptr<Material> blitDepthMaterial;

		size_t nDrawCalls = 0;
		size_t nVertices = 0;
		size_t nTriangles = 0;
		size_t prevDrawCalls = 0;
		size_t prevVertices = 0;
		size_t prevTriangles = 0;
		bool logging = true;

		Vector<IndexType> stdQuadIndexCache;
		std::optional<Rect4i> curClip;
		std::optional<Rect4i> pendingClip;

		Vector<String> curDebugGroupStack;
		Vector<String> pendingDebugGroupStack;

		HashMap<uint64_t, ConstantBufferEntry> constantBuffers;
		std::shared_ptr<MaterialShaderStorageBuffer> objectDataBuffer;

		RenderSnapshot* recordingSnapshot = nullptr;
		bool recordingPerformance = false;
		uint64_t frameStart, frameEnd;
		std::chrono::steady_clock::time_point frameStartCPUTime;

		void bind(RenderContext& context);
		void unbind(RenderContext& context);
		void doBind(const Camera& camera, RenderTarget& renderTarget);
		void doUnbind();
		
		void startRender();
		void endRender();
		
		void resetPending();
		void startDrawCall(const std::shared_ptr<const Material>& material);
		void flushPending();
		void executeDrawPrimitives(const Material& material, size_t numObjects, size_t numVertices, gsl::span<const char> objectData, gsl::span<const char> vertexData, gsl::span<const IndexType> indices, PrimitiveType primitiveType, bool allIndicesAreQuads);

		void makeSpaceForPendingVertices(size_t numBytes);
		void makeSpaceForPendingObjects(size_t numBytes);
		void makeSpaceForPendingIndices(size_t numIndices);
		PainterVertexData addDrawData(const std::shared_ptr<const Material>& material, size_t numObjects, size_t numVertices, size_t numIndices, bool standardQuadsOnly);

		IndexType* getStandardQuadIndices(size_t numQuads);
		void generateQuadIndicesOffset(IndexType firstVertex, IndexType lineStride, IndexType* target);

		void updateProjection();
		void updateClip();

		Rect4i getRectangleForActiveRenderTarget(Rect4i rectangle);

		std::shared_ptr<const Material> getSolidLineMaterial();
		std::shared_ptr<const Material> getSolidPolygonMaterial();

		void refreshConstantBufferCache();

		size_t getMaxObjects(const MaterialDefinition& material) const;
	};
}
