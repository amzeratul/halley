#pragma once

#include <type_traits>

#include "halley/maths/polygon.h"
#include "halley/data_structures/tree_map.h"

namespace Halley {
	class ScriptState;

	struct DebugLine
	{
		DebugLine(Vector<Vector2f> points, Colour4f colour, float thickness, bool loop)
			: points(std::move(points))
			, colour(colour)
			, thickness(thickness)
			, loop(loop)
		{
		}

		Vector<Vector2f> points;
		Colour4f colour;
		float thickness;
		bool loop;
	};

	struct DebugPoint
	{
		DebugPoint(Vector2f point, Colour4f colour, float radius)
			: point(point)
			, colour(colour)
			, radius(radius)
		{
		}

		Vector2f point;
		Colour4f colour;
		float radius;
	};

	struct DebugPolygon {
		DebugPolygon(Polygon polygon, Colour4f colour)
			: polygon(std::move(polygon))
			, colour(colour)
		{}

		Polygon polygon;
		Colour4f colour;
	};

	struct DebugEllipse {
		DebugEllipse(Vector2f centre, Vector2f radius, float width, Colour4f colour)
			: centre(centre)
			, radius(radius)
			, width(width)
			, colour(colour)
		{}

		Vector2f centre;
		Vector2f radius;
		float width;
		Colour4f colour;
	};

	struct DebugText {
		DebugText(String text = {})
			: text(std::move(text))
		{}

		String text;
		Time time = 0;
	};


	class BaseFrameData {
	public:
		virtual ~BaseFrameData() = default;

		static void setThreadFrameData(BaseFrameData* value)
		{
			threadInstance = value;
		}
		
		static BaseFrameData& getCurrent()
		{
			assert(hasCurrent());
			return *threadInstance;
		}

		static bool hasCurrent()
		{
			return threadInstance != nullptr;
		}

		virtual void doStartFrame(bool multithreaded, BaseFrameData* previous, Time deltaTime) {}


		Vector<DebugLine> debugLines;
		Vector<DebugPoint> debugPoints;
		Vector<DebugPolygon> debugPolygons;
		Vector<DebugEllipse> debugEllipses;
		TreeMap<String, DebugText> debugTexts;
		Vector<std::pair<Vector2f, std::shared_ptr<ScriptState>>> scriptStates;

	protected:
		static thread_local BaseFrameData* threadInstance;
	};

	template <typename T>
	class FrameData : public BaseFrameData {
	public:
		virtual void startFrame(bool multithreaded, T* previous, Time deltaTime) {}

		static T& getCurrent()
		{
			assert(hasCurrent());
			return static_cast<T&>(*threadInstance);
		}

	protected:
		void doStartFrame(bool multithreaded, BaseFrameData* previous, Time deltaTime) override
		{
			startFrame(multithreaded, static_cast<T*>(previous), deltaTime);
		}
	};

	class EmptyFrameData : public FrameData<EmptyFrameData> {
	public:
	};
}
