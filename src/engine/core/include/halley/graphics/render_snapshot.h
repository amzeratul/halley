#pragma once
#include "camera.h"
#include "graphics_enums.h"
#include "material/material.h"

namespace Halley {
    class ITimestampRecorder {
    public:
        virtual ~ITimestampRecorder() = default;
        virtual void addPendingTimestamp() = 0;
        virtual void onTimestamp(TimestampType type, size_t idx, uint64_t value) = 0;
    };

    class RenderContext;
    class RenderTarget;

	enum class TargetBufferType {
		Colour,
		Depth,
		Stencil
	};

    class RenderSnapshot : public ITimestampRecorder {
    public:
    	enum class CommandType : uint8_t {
            Undefined,
	        Bind,
            Unbind,
            SetClip,
            Clear,
            Draw
        };

        enum class Reason : uint8_t {
            Unknown,
	        First,
            Clear,
            AfterClear,
            ChangeBind,
            ChangeClip,
            MaterialDefinition,
            MaterialParameters,
            Textures
        };

        struct ClearData {
            std::optional<Colour4f> colour;
            std::optional<float> depth;
            std::optional<uint8_t> stencil;
        };

        struct CommandInfo {
            CommandType type = CommandType::Undefined;
            Reason reason = Reason::Unknown;
            bool hasBindChange = false;
            bool hasClipChange = false;
            bool hasMaterialDefChange = false;
            bool hasMaterialParamsChange = false;
            bool hasTextureChange = false;
            size_t numTriangles = 0;
            uint64_t materialHash = 0;
            String materialDefinition;
            Vector<String> textures;
            std::optional<ClearData> clearData;
        };

        struct PlaybackResult {
            String finalRenderTargetName;
        };

        struct CommandTimeStamp {
            uint64_t start = 0;
            uint64_t end = 0;
            uint64_t setupDone = 0;
        };

        RenderSnapshot();

        void start();
        void end();

        void bind(RenderContext& context);
        void unbind(RenderContext& context);

        void setClip(Rect4i rect, bool enable);
    	void clear(std::optional<Colour4f> colour, std::optional<float> depth, std::optional<uint8_t> stencil);
	    void draw(const Material& material, size_t numObjects, size_t numVertices, gsl::span<const char> objectData, gsl::span<const char> vertexData, gsl::span<const IndexType> indices, PrimitiveType primitive, bool allIndicesAreQuads);

        void finish();

        size_t getNumCommands() const;
        CommandInfo getCommandInfo(size_t commandIdx) const;

        PlaybackResult playback(Painter& painter, std::optional<size_t> maxCommands, TargetBufferType blitType = TargetBufferType::Colour, std::shared_ptr<const MaterialDefinition> debugMaterial = {}) const;

        void addPendingTimestamp() override;
        void onTimestamp(TimestampType type, size_t idx, uint64_t value) override;

    	bool hasPendingTimestamps() const;
        size_t getNumTimestamps() const;
        std::pair<uint64_t, uint64_t> getFrameTimeRange() const;
        const CommandTimeStamp& getCommandTimeStamp(size_t idx) const;

    private:
        struct BindData {
            Camera camera;
            RenderTarget* renderTarget;
        };

        struct SetClipData {
            Rect4i rect;
            bool enable;
        };

        struct DrawData {
            const Material* materialTemp;
            std::shared_ptr<Material> material;
            size_t numObjects;
            size_t numVertices;
            Vector<char> vertexData;
            Vector<char> objectData;
            Vector<IndexType> indices;
            PrimitiveType primitive;
            bool allIndicesAreQuads;
        };

        Vector<Vector<std::pair<CommandType, uint16_t>>> commands;
        Vector<BindData> bindDatas;
        Vector<SetClipData> setClipDatas;
        Vector<ClearData> clearDatas;
        Vector<DrawData> drawDatas;

    	std::atomic<int> pendingTimestamps;
        uint64_t startTime = 0;
        uint64_t endTime = 0;
        Vector<CommandTimeStamp> timestamps;

        Vector<std::pair<CommandType, uint16_t>>& getCurDrawCall();
        void finishDrawCall();

        void playBind(Painter& painter, const BindData& data) const;
        void playUnbind(Painter& painter) const;
        void playClear(Painter& painter, const ClearData& data) const;
        void playSetClip(Painter& painter, const SetClipData& data) const;
        void playDraw(Painter& painter, const DrawData& data, std::shared_ptr<const MaterialDefinition> debugMaterial) const;
    };
}
