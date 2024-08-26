#include <systems/camera_render_system.h>

using namespace Halley;

class CameraRenderSystem final : public CameraRenderSystemBase<CameraRenderSystem>, public IScreenGrabInterface {
public:
	void init()
	{
		getScreenService().setScreenGrabInterface(this);

		if (!getPainterService().hasRenderGraph()) {
			getPainterService().setRenderGraph(std::make_unique<RenderGraph>(getResources().get<RenderGraphDefinition>("default_render_graph")));
		}
	}

	void render(RenderContext& rc)
	{
		getPainterService().startRender(!getDevService().isEditorMode());

		if (getPainterService().hasRenderGraph()) {
			auto& renderGraph = getPainterService().getRenderGraph();

			setupRenderGraphCameras(rc.getDefaultRenderTarget().getViewPort(), renderGraph);
			initializeRequestedScreenGrabs(renderGraph);
			executeRenderGraphCommands(renderGraph);
			renderGraph.render(rc, *getAPI().video);
			cleanUpScreenGrabs(renderGraph);
			renderGlobalScreenGrab(rc, renderGraph);
		}

		getPainterService().endRender();
	}

	Future<std::unique_ptr<Image>> requestScreenGrab(std::optional<Rect4i> rect, ScreenGrabMode mode) override
	{
		std::unique_lock<std::mutex> lock(captureMutex);
		auto& pc = queuedCaptures.emplace_back(PendingCapture{ rect, mode });
		return pc.promise.getFuture();
	}

	Future<std::unique_ptr<Image>> requestGlobalScreenGrab(Rect4i worldRect, ScreenGrabMode mode, float zoom) override
	{
		pendingGlobalCapture = PendingCapture();
		pendingGlobalCapture->rect = worldRect;
		pendingGlobalCapture->mode = mode;
		pendingGlobalCapture->zoom = zoom;
		return pendingGlobalCapture->promise.getFuture();
	}

private:
	struct PendingCapture {
		std::optional<Rect4i> rect;
		ScreenGrabMode mode;
		Promise<std::unique_ptr<Image>> promise;
		bool fulfilled = false;
		float zoom = 1.0f;
	};
	Vector<PendingCapture> pendingCaptures;
	Vector<PendingCapture> queuedCaptures;
	std::optional<PendingCapture> pendingGlobalCapture;
	std::mutex captureMutex;

	void setupRenderGraphCameras(Rect4i viewPort, RenderGraph& renderGraph)
	{
		// Prepare cameras
		const auto size = viewPort.getSize();
		renderGraph.clearCameras();

		for (auto& [id, camera]: getPainterService().getFrameData().cameras) {
			camera.setPosition(getScreenService().roundCameraPosition(camera.getPosition().xy(), size));
			renderGraph.setCamera(id, camera);
		}
	}

	void initializeRequestedScreenGrabs(RenderGraph& renderGraph)
	{
		{
			std::unique_lock<std::mutex> lock(captureMutex);
			pendingCaptures = std::move(queuedCaptures);
			queuedCaptures.clear();
		}
		
		std::set<ScreenGrabMode> modesToCapture;
		for (const auto& pc: pendingCaptures) {
			modesToCapture.insert(pc.mode);
		}
		for (const auto& mode: modesToCapture) {
			renderGraph.setImageOutputCallback(toString(mode), [=] (Image& image) { onImageCaptured(mode, image); });
		}
	}

	void executeRenderGraphCommands(RenderGraph& renderGraph)
	{
		for (const auto& command : getPainterService().getFrameData().renderGraphCommands) {
			command->apply(renderGraph);
		}
		getPainterService().getFrameData().renderGraphCommands.clear();
	}

	void cleanUpScreenGrabs(RenderGraph& renderGraph)
	{
		renderGraph.clearImageOutputCallbacks();
		for (auto& pc: pendingCaptures) {
			if (!pc.fulfilled) {
				pc.promise.setValue({});
			}
		}
		pendingCaptures.clear();
	}

	void onImageCaptured(ScreenGrabMode mode, Image& image)
	{
		const int downscale = lroundl(getScreenService().getZoomLevel());

		for (auto& pc: pendingCaptures) {
			if (pc.mode == mode) {
				auto downscaledImage = std::make_unique<Image>(image.getFormat(), image.getSize() / downscale);
				downscaledImage->blitDownsampled(image, downscale);
				const auto downscaledRect = downscaledImage->getRect();
				
				if (!pc.rect || pc.rect == downscaledRect) {
					pc.promise.setValue(std::move(downscaledImage));
				} else {
					auto crop = pc.rect.value().intersection(downscaledRect);
					
					auto scaledImage = std::make_unique<Image>(image.getFormat(), crop.getSize());
					scaledImage->blitFrom(Vector2i(), *downscaledImage, crop);
					pc.promise.setValue(std::move(scaledImage));
				}
				pc.fulfilled = true;
			}
		}
	}

	void renderGlobalScreenGrab(RenderContext& rc, RenderGraph& renderGraph)
	{
		if (!pendingGlobalCapture) {
			return;
		}
		const auto mode = pendingGlobalCapture->mode;

		// Determine viewport and create image
		const float zoom = pendingGlobalCapture->zoom;
		const auto viewPort = pendingGlobalCapture->rect.value();
		const auto screenTileSize = Vector2i(2048, 2048);
		const auto worldTileSize = Vector2i((Vector2f(screenTileSize) / zoom).round());
		const auto imageSize = Vector2i((Vector2f(viewPort.getSize()) * zoom).round());
		const auto numTiles = (imageSize + (screenTileSize - Vector2i(1, 1))) / screenTileSize;
		auto finalImage = std::make_unique<Image>(Image::Format::RGBA, imageSize);

		// Perform all screen grabs
		for (int y = 0; y < numTiles.y; ++y) {
			for (int x = 0; x < numTiles.x; ++x) {
				// Position
				const auto topLeft = Vector2i(x, y) * worldTileSize;
				
				// Setup camera
				Camera camera;
				camera.setZoom(zoom);
				camera.setPosition(Vector2f(topLeft + worldTileSize / 2 + viewPort.getTopLeft()));
				renderGraph.setCamera("main", camera);

				// Set callback
				renderGraph.setImageOutputCallback(toString(mode), [&] (Image& image)
				{
					finalImage->blitFrom(topLeft, image);
				});

				// Render
				renderGraph.render(rc, *getAPI().video, screenTileSize);
			}
		}

		// Finish capture
		renderGraph.clearImageOutputCallbacks();
		pendingGlobalCapture->promise.setValue(std::move(finalImage));
		pendingGlobalCapture.reset();
	}
};

REGISTER_SYSTEM(CameraRenderSystem)

