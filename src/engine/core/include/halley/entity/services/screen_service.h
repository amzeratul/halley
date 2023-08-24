#pragma once

#include "halley/entity/service.h"
#include "halley/concurrency/future.h"
#include "halley/file_formats/image.h"
#include "halley/maths/rect.h"
#include "halley/maths/vector2.h"

namespace Halley {

	class IScreenServiceInterface {
	public:
		virtual ~IScreenServiceInterface() = default;

		virtual Vector2i getGameResolution() = 0;
		virtual Vector2i getScreenResolution() const = 0;
		virtual Vector2i getUIResolution() = 0;
		virtual float getZoomLevel() const = 0;
	};

	enum class ScreenGrabMode {
		ComposedWithUI,
		ComposedNoUI
	};
	
	template <>
	struct EnumNames<ScreenGrabMode> {
		constexpr std::array<const char*, 2> operator()() const {
			return{ {
				"composedWithUI",
				"composedNoUI"
			} };
		}
	};

	class IScreenGrabInterface {
	public:
		virtual ~IScreenGrabInterface() = default;

		virtual Future<std::unique_ptr<Image>> requestScreenGrab(std::optional<Rect4i> rect, ScreenGrabMode mode) = 0;
		virtual Future<std::unique_ptr<Image>> requestGlobalScreenGrab(Rect4i worldRect, ScreenGrabMode mode, float zoom = 1.0f) = 0;
	};

	class ScreenService : public Service, public IScreenGrabInterface {
	public:
		explicit ScreenService(IScreenServiceInterface* interface = nullptr);
		explicit ScreenService(std::shared_ptr<IScreenServiceInterface> interface);

		bool hasInterface() const;

		Vector2i getGameResolution() const;
		Vector2i getScreenResolution() const;
		Vector2i getUIResolution() const;
		float getZoomLevel() const;

		Rect4f getCameraViewPort() const;
		void setCameraPosition(Vector2f camPos);

		Vector2f worldToScreen(Vector2f pos) const;
		Vector2f worldToUI(Vector2f pos) const;
		Vector2f screenToWorld(Vector2f pos) const;

		Vector2f roundPosition(Vector2f pos) const
		{
			const float zoom = std::max(1.0f, getZoomLevel());
			return roundPosition(pos, zoom);
		}

		static Vector2f roundPosition(Vector2f pos, float zoom)
		{
			const float effectiveZoom = std::max(1.0f, zoom);
			return (pos * effectiveZoom).floor() / effectiveZoom;
		}

		Vector2f roundCameraPosition(Vector2f pos, Vector2i screenResolution) const
		{
			return roundCameraPosition(pos, screenResolution, getZoomLevel());
		}

		static Vector2f roundCameraPosition(Vector2f pos, Vector2i screenResolution, float zoom)
		{
			const float z = std::max(1.0f, zoom);
			const auto offset = Vector2f(screenResolution % Vector2i(2, 2)) * 0.5f;
			return roundPosition(pos, z) + offset / z;
		}

		void setScreenGrabInterface(IScreenGrabInterface* interface);
		Future<std::unique_ptr<Image>> requestScreenGrab(std::optional<Rect4i> rect = {}, ScreenGrabMode mode = ScreenGrabMode::ComposedWithUI) override;
		Future<std::unique_ptr<Image>> requestGlobalScreenGrab(Rect4i worldRect, ScreenGrabMode mode = ScreenGrabMode::ComposedNoUI, float zoom = 1.0f) override;
		bool isScreenGrabMode() const;

		std::shared_ptr<IScreenServiceInterface> getInterfacePointer();

	private:
		IScreenServiceInterface* interface = nullptr;
		std::shared_ptr<IScreenServiceInterface> interfacePtr;
		IScreenGrabInterface* screenGrabInterface = nullptr;
		Vector2f cameraPosition;
		bool screenGrabMode = false;
	};
}

using ScreenService = Halley::ScreenService;
