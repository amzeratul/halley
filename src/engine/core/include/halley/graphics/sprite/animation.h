#pragma once

#include <halley/text/halleystring.h>
#include <memory>
#include <halley/resources/resource.h>
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"

namespace Halley
{
	class Animation;
	class Deserializer;
	class Serializer;
	class ResourceLoader;
	class SpriteSheet;
	class SpriteSheetEntry;
	class Material;
	class AnimationDirection;
	class AnimationImporter;

	class AnimationFrame
	{
	public:
		AnimationFrame(int frameNumber, int duration, const String& imageName, const SpriteSheet& sheet, const Vector<AnimationDirection>& directions);

		const SpriteSheetEntry& getSprite(int dir) const
		{
			Expects(dir >= 0 && dir < int(sprites.size()));
			return *sprites[dir];
		}

		int getDuration() const;

	private:
		Vector<const SpriteSheetEntry*> sprites;
		int duration;
	};

	class AnimationFrameDefinition
	{
	public:
		AnimationFrameDefinition();
		AnimationFrameDefinition(int frameNumber, int duration, String imageName);
		AnimationFrame makeFrame(const SpriteSheet& sheet, const Vector<AnimationDirection>& directions) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		String imageName;
		int frameNumber;
		int duration;
	};

	class AnimationSequence
	{
		friend class Animation;

	public:
		AnimationSequence();
		AnimationSequence(String name, bool loop, bool noFlip, bool fallback);

		size_t numFrames() const { return frames.size(); }
		size_t numFrameDefinitions() const { return frameDefinitions.size(); }
		const AnimationFrame& getFrame(size_t n) const
		{
			Expects(n < frames.size());
			return frames[n];
		}
		const String& getName() const { return name; }
		int getId() const { return id; }
		bool isLooping() const { return loop; }
		bool isNoFlip() const { return noFlip; }
		bool isFallback() const { return fallback; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void addFrame(const AnimationFrameDefinition& animationFrameDefinition);

		Rect4i getBounds() const;

	private:
		Vector<AnimationFrame> frames;
		Vector<AnimationFrameDefinition> frameDefinitions;
		String name;
		int id;
		bool loop = false;
		bool noFlip = false;
		bool fallback = false;
	};

	class AnimationDirection
	{
		friend class Animation;

	public:
		AnimationDirection();
		AnimationDirection(String name, String fileName, bool flip);

		const String& getName() const { return name; }
		const String& getFileName() const { return fileName; }
		bool shouldFlip() const { return flip; }
		int getId() const { return id; }

		bool needsToProcessFrameName(const String& baseName) const;
		String getFrameName(int frameNumber, String baseName) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		String name;
		String fileName;
		int id = -1;
		bool flip = false;
	};

	class AnimationActionPoint {
	public:
		AnimationActionPoint() = default;
		AnimationActionPoint(const ConfigNode& config, String name, int id, gsl::span<const AnimationSequence> sequences, gsl::span<const AnimationDirection> directions);

		const String& getName() const { return name; }
		int getId() const { return id; }
		std::optional<Vector2i> getPoint(int sequenceIdx, int directionIdx, int frameNumber) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		String name;
		int id = -1;
		HashMap<std::tuple<int, int, int>, Vector2i> points; // Key is (SequenceIdx, DirectionIdx, FrameNumber)
	};
	
	class Animation final : public Resource
	{
		friend class AnimationImporter;

	public:
		Animation();

		static std::unique_ptr<Animation> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Animation; }
		void reload(Resource&& resource) override;

		const String& getName() const { return name; }
		const SpriteSheet& getSpriteSheet() const { return *spriteSheet; }
		std::shared_ptr<Material> getMaterial() const { return material; }

		const AnimationSequence& getSequence(const String& name) const;
		const AnimationSequence& getSequence(size_t idx) const;
		size_t getSequenceIdx(const String& name) const;
		const AnimationDirection& getDirection(const String& name) const;
		const AnimationDirection& getDirection(int id) const;
		Vector<String> getSequenceNames() const;
		Vector<String> getDirectionNames() const;

		std::optional<Vector2i> getActionPoint(const String& actionPoint, const String& sequenceName, const String& directionName, int frameNumber) const;
		std::optional<Vector2i> getActionPoint(const String& actionPoint, int sequenceIdx, int directionIdx, int frameNumber) const;
		
		Vector2i getPivot() const;
		Rect4i getBounds() const;

		bool hasSequence(const String& name) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
		void loadDependencies(ResourceLoader& loader);

		void setName(const String& name);
		void setMaterialName(const String& name);
		void setSpriteSheetName(const String& name);
		void addSequence(AnimationSequence sequence);
		void addDirection(AnimationDirection direction, std::optional<int> idx = {});
		void addActionPoints(const ConfigNode& config);

	private:
		mutable bool hasPivot = false;
		mutable bool hasBounds = false;
		mutable Vector2i pivot;
		mutable Rect4i bounds;

		std::shared_ptr<const SpriteSheet> spriteSheet;
		std::shared_ptr<Material> material;

		Vector<AnimationSequence> sequences;
		Vector<AnimationDirection> directions;
		Vector<AnimationActionPoint> actionPoints;

		String name;
		String spriteSheetName;
		String materialName;
	};
}
