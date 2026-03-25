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

		virtual Vector2i getGameResolution() const = 0;
		virtual Vector2i getScreenResolution() const = 0;
		virtual Vector2i getUIResolution() const = 0;
		virtual float getWorldZoomLevel() const = 0;
		virtual float getUIZoomLevel() const { return getWorldZoomLevel(); }
		virtual void setWorldZoomMultiplier(float zoom) {}
	};

	enum class ScreenGrabMode {
		ComposedWithUI,
		ComposedNoUI
	};
	
	template <>
	struct EnumNames<ScreenGrabMode> {
		constexpr std::array<const char*, 2> operator()() const {
			return std::to_array({
				"composedWithUI",
				"composedNoUI"
			});
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
		float getUIZoomLevel() const;
		float getWorldZoomLevel() const;

		Rect4f getCameraViewPort() const;
		static Rect4f getCameraViewPort(Vector2f cameraPos, Vector2i gameRes);
		void setCameraPosition(Vector2f camPos);
		bool isVisible(const Rect4f& aabb) const;

		Vector2f worldToScreen(Vector2f pos) const;
		Vector2f worldToUI(Vector2f pos) const;
		Vector2f screenToWorld(Vector2f pos) const;
		Vector2f screenToUI(Vector2f pos) const;
		Vector2f uiToScreen(Vector2f pos) const;
		Vector2f uiToWorld(Vector2f pos) const;

		Vector2f roundUIPosition(Vector2f pos) const
		{
			const float zoom = std::max(1.0f, getUIZoomLevel());
			return roundPosition(pos, zoom);
		}

		Vector2f roundWorldPosition(Vector2f pos) const
		{
			const float zoom = std::max(1.0f, getWorldZoomLevel());
			return roundPosition(pos, zoom);
		}

		constexpr inline static Vector2f roundPosition(Vector2f pos, float zoom)
		{
			const float effectiveZoom = std::max(1.0f, zoom);
			return (pos * effectiveZoom + Vector2f(0.5f, 0.5f)).floor() / effectiveZoom;
		}

		Vector2f roundUICameraPosition(Vector2f pos, Vector2i screenResolution) const
		{
			return roundCameraPosition(pos, screenResolution, getUIZoomLevel());
		}

		Vector2f roundWorldCameraPosition(Vector2f pos, Vector2i screenResolution) const
		{
			return roundCameraPosition(pos, screenResolution, getWorldZoomLevel());
		}

		constexpr static Vector2f roundCameraPosition(Vector2f pos, Vector2i screenResolution, float zoom)
		{
			const float z = std::max(1.0f, zoom);
			const auto offset = Vector2f(screenResolution % Vector2i(2, 2)) * 0.5f;
			return roundPosition(pos, z) + offset / z;
		}

		void setScreenGrabInterface(IScreenGrabInterface* interface);
		Future<std::unique_ptr<Image>> requestScreenGrab(std::optional<Rect4i> rect = {}, ScreenGrabMode mode = ScreenGrabMode::ComposedWithUI) override;
		Future<std::unique_ptr<Image>> requestGlobalScreenGrab(Rect4i worldRect, ScreenGrabMode mode = ScreenGrabMode::ComposedNoUI, float zoom = 1.0f) override;
		bool isScreenGrabMode() const;
		Rect4i getScreenGrabRect() const;

		void setWorldZoomMultiplier(float zoom);

		std::shared_ptr<IScreenServiceInterface> getInterfacePointer();

	private:
		IScreenServiceInterface* interface = nullptr;
		std::shared_ptr<IScreenServiceInterface> interfacePtr;
		IScreenGrabInterface* screenGrabInterface = nullptr;
		Vector2f cameraPosition;
		bool screenGrabMode = false;
		Rect4i screenGrabRect;
	};
}

using ScreenService = Halley::ScreenService;
