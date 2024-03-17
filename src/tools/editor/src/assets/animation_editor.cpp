#include "animation_editor.h"

#include "metadata_editor.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/tools/project/project.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "src/ui/scroll_background.h"

using namespace Halley;

AnimationEditor::AnimationEditor(UIFactory& factory, Resources& gameResources, AssetType type, Project& project, MetadataEditor& metadataEditor)
	: AssetEditor(factory, gameResources, project, type)
	, metadataEditor(metadataEditor)
{
}

void AnimationEditor::onResourceLoaded()
{
	setupWindow();
	loadAssetData();
}

void AnimationEditor::refreshAssets()
{
	AssetEditor::refreshAssets();
	if (resource) {
		loadAssetData();
	}
}

bool AnimationEditor::isReadyToLoad() const
{
	return project.areAssetsLoaded();
}

void AnimationEditor::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this());
}

void AnimationEditor::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

bool AnimationEditor::onKeyPress(KeyboardKeyPress key)
{
	if (!animationDisplay) {
		return false;
	}

	if (key.is(KeyCode::Space)) {
		togglePlay();
		return true;
	}

	if (key.is(KeyCode::Left)) {
		animationDisplay->prevFrame();
		return true;
	}

	if (key.is(KeyCode::Right)) {
		animationDisplay->nextFrame();
		return true;
	}

	return false;
}

void AnimationEditor::update(Time t, bool moved)
{
	AssetEditor::update(t, moved);

	if (!animationDisplay) {
		return;
	}

	const auto mousePos = Vector2i(animationDisplay->getMousePos());
	const auto size = animationDisplay->getBounds().getSize();
	String str = "Frame: " + toString(animationDisplay->getFrameNumber()) +  ", x: " + toString(mousePos.x) + " y: " + toString(mousePos.y) + " (" + toString(size.x) + "x" + toString(size.y) + ")";

#ifdef ENABLE_HOT_RELOAD
	const auto spriteSheet = std::dynamic_pointer_cast<const SpriteSheet>(resource);
	if (spriteSheet) {
		if (const SpriteSheetEntry* result = spriteSheet->getSpriteAtTexel(mousePos)) {
			str += "\nSprite: " + result->name;
		} else {
			str += "\nSprite: N/A";
		}
	}
#endif

	info->setText(LocalisedString::fromUserString(str));
}

std::shared_ptr<const Resource> AnimationEditor::loadResource(const Path& assetPath, const String& assetId, AssetType assetType)
{
	std::shared_ptr<const Resource> resource;
	if (assetType == AssetType::Animation) {
		resource = gameResources.get<Animation>(assetId);
	} else if (assetType == AssetType::Sprite) {
		resource = gameResources.get<SpriteResource>(assetId);
	} else if (assetType == AssetType::Texture) {
		resource = gameResources.get<Texture>(assetId);
	} else if (assetType == AssetType::SpriteSheet) {
		resource = gameResources.get<SpriteSheet>(assetId);
	}

	return resource;
}

void AnimationEditor::setupWindow()
{
	add(factory.makeUI("halley/animation_editor"), 1);
	animationDisplay = getWidgetAs<AnimationEditorDisplay>("display");
	animationDisplay->setMetadataEditor(metadataEditor);
	
	info = getWidgetAs<UILabel>("info");
	scrollBg = getWidgetAs<ScrollBackground>("scrollBackground");

	if (assetType == AssetType::Animation || assetType == AssetType::Sprite) {
		updateActionPointList();
		getWidget("topBar")->setActive(true);
	} else {
		getWidget("topBar")->setActive(false);
	}

	scrollBg->setZoomListener([=] (float zoom)
	{
		animationDisplay->setZoom(zoom);
	});

	scrollBg->setMousePosListener([=] (Vector2f mousePos, KeyMods keyMods)
	{
		animationDisplay->onMouseOver(mousePos, keyMods);
	});

	setHandle(UIEventType::CanvasDoubleClicked, "scrollBackground", [=] (const UIEvent& event)
	{
		if (assetType == AssetType::Animation || assetType == AssetType::Sprite) {
			animationDisplay->onDoubleClick();
		}
	});

	setHandle(UIEventType::ButtonClicked, "play", [=] (const UIEvent& event)
	{
		togglePlay();
	});

	setHandle(UIEventType::ButtonClicked, "prevFrame", [=] (const UIEvent& event)
	{
		animationDisplay->prevFrame();
	});

	setHandle(UIEventType::ButtonClicked, "nextFrame", [=] (const UIEvent& event)
	{
		animationDisplay->nextFrame();
	});

	setHandle(UIEventType::DropdownSelectionChanged, "sequence", [=] (const UIEvent& event)
	{
		animationDisplay->setSequence(event.getStringData());
	});

	setHandle(UIEventType::DropdownSelectionChanged, "direction", [=] (const UIEvent& event)
	{
		animationDisplay->setDirection(event.getStringData());
	});

	setHandle(UIEventType::DropdownSelectionChanged, "actionPoints", [=] (const UIEvent& event)
	{
		animationDisplay->setActionPoint(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "addPoint", [=] (const UIEvent& event)
	{
		// TODO
		//animationDisplay->addPoint();
	});

	setHandle(UIEventType::ButtonClicked, "removePoint", [=] (const UIEvent& event)
	{
		// TODO
		//animationDisplay->removePoint();
	});

	setHandle(UIEventType::ButtonClicked, "clearPoint", [=] (const UIEvent& event)
	{
		animationDisplay->clearPoint();
	});

	updatePlayIcon();
}

void AnimationEditor::loadAssetData()
{
	const auto animation = std::dynamic_pointer_cast<const Animation>(resource);
	const auto sprite = std::dynamic_pointer_cast<const SpriteResource>(resource);
	const auto texture = std::dynamic_pointer_cast<const Texture>(resource);
	const auto spriteSheet = std::dynamic_pointer_cast<const SpriteSheet>(resource);

	if (animation) {
		animationDisplay->setAnimation(animation);
	} else if (sprite) {
		animationDisplay->setSprite(sprite);
	} else if (texture) {
		animationDisplay->setTexture(texture);
	} else if (spriteSheet) {
		animationDisplay->setTexture(spriteSheet->getTexture());
	}

	if (animation) {
		auto sequenceList = getWidgetAs<UIDropdown>("sequence");
		sequenceList->setOptions(animation->getSequenceNames());

		auto directionList = getWidgetAs<UIDropdown>("direction");
		directionList->setOptions(animation->getDirectionNames());
	} else {
		getWidget("animControls")->setActive(false);
	}
}

void AnimationEditor::togglePlay()
{
	animationDisplay->setPlaying(!animationDisplay->isPlaying());
	updatePlayIcon();
}

void AnimationEditor::updatePlayIcon()
{
	getWidgetAs<UIButton>("play")->setIcon(Sprite().setImage(factory.getResources(), animationDisplay->isPlaying() ? "halley_ui/icon_pause.png" : "halley_ui/icon_play.png"));
}

void AnimationEditor::updateActionPointList()
{
	Vector<String> actionPointIds;

	auto actionPointData = metadataEditor.getValue("actionPoints");
	if (actionPointData.getType() == ConfigNodeType::Map) {
		for (const auto& [k, v]: actionPointData.asMap()) {
			actionPointIds.push_back(k);
		}
	}
	std::sort(actionPointIds.begin(), actionPointIds.end());
	actionPointIds.insert(actionPointIds.begin(), "pivot");

	getWidgetAs<UIDropdown>("actionPoints")->setOptions(actionPointIds);
}

AnimationEditorDisplay::AnimationEditorDisplay(String id, Resources& resources)
	: UIWidget(std::move(id))
	, resources(resources)
{
	boundsSprite.setImage(resources, "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	nineSliceVSprite.setImage(resources, "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	nineSliceHSprite.setImage(resources, "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	actionPointSprite.setImage(resources, "ui/pivot.png").setColour(Colour4f(1, 0, 1));

	actionPointId = "pivot";
}

void AnimationEditorDisplay::setZoom(float z)
{
	zoom = z;
	updateBounds();
}

void AnimationEditorDisplay::setAnimation(std::shared_ptr<const Animation> a)
{
	animation = std::move(a);
	animationPlayer.setAnimation(animation);
	origBounds = animation->getBounds();
	origPivot = animation->getPivot();
	updateBounds();
}

void AnimationEditorDisplay::setSprite(std::shared_ptr<const SpriteResource> sprite)
{
	origSprite.setImage(*sprite, resources.get<MaterialDefinition>(MaterialDefinition::defaultMaterial));
	const auto pivot = Vector2i(origSprite.getAbsolutePivot());
	const auto origin = -pivot - Vector2i(origSprite.getOuterBorder().xy());
	const auto sz = Vector2i(origSprite.getUncroppedSize());

	origPivot = pivot;
	origBounds = Rect4i(origin, origin + sz);
	updateBounds();
}

void AnimationEditorDisplay::setTexture(std::shared_ptr<const Texture> texture)
{
	origSprite.setImage(texture, resources.get<MaterialDefinition>(MaterialDefinition::defaultMaterial))
		.setTexRect(Rect4f(0, 0, 1, 1))
		.setColour(Colour4f(1, 1, 1, 1))
		.setSize(Vector2f(texture->getSize()));
	origPivot.reset();
	origBounds = Rect4i(Vector2i(), Vector2i(origSprite.getSize()));
	updateBounds();
}

void AnimationEditorDisplay::setSequence(const String& sequence)
{
	animationPlayer.setSequence(sequence);
}

void AnimationEditorDisplay::setDirection(const String& direction)
{
	animationPlayer.setDirection(direction);
}

void AnimationEditorDisplay::refresh()
{
	updateBounds();
}

const Rect4f& AnimationEditorDisplay::getBounds() const
{
	return bounds;
}

Vector2f AnimationEditorDisplay::getMousePos() const
{
	return mousePos;
}

void AnimationEditorDisplay::update(Time t, bool moved)
{
	updateBounds();

	if (animation) {
		animationPlayer.update(t);
		animationPlayer.updateSprite(origSprite);
	}

	const Vector2f pivotPos = imageToScreenSpace(-bounds.getTopLeft());
	drawSprite = origSprite.clone().setPos(pivotPos).setScale(zoom).setNotSliced();
	boundsSprite.setPos(getPosition()).scaleTo((bounds.getSize() * zoom).round());

	const auto actionPointPos = getCurrentActionPoint();
	if (actionPointPos) {
		const Vector2f displayPivotPos = imageToScreenSpace(-bounds.getTopLeft() + Vector2f(*actionPointPos));
		actionPointSprite.setPos(displayPivotPos);
		actionPointSprite.setVisible(true);
	} else {
		actionPointSprite.setVisible(false);
	}

	const auto slices = getCurrentSlices();
	if (slices) {
		nineSliceVSprite.setVisible(true).setPos(getPosition() + Vector2f(0, slices->y) * zoom).scaleTo(Vector2f::max(Vector2f(1, 1), (bounds.getSize() - Vector2f(0, slices->w + slices->y)) * zoom));
		nineSliceHSprite.setVisible(true).setPos(getPosition() + Vector2f(slices->x, 0) * zoom).scaleTo(Vector2f::max(Vector2f(1, 1), (bounds.getSize() - Vector2f(slices->x + slices->z, 0)) * zoom));
	} else {
		nineSliceVSprite.setVisible(false);
		nineSliceHSprite.setVisible(false);
	}
}

namespace {
	Vector2f findPoint(Vector2f origin, Angle1f dir, Rect4f bounds)
	{
		const auto ray = Ray(origin, Vector2f(1, 0).rotate(dir));
		Vector<Ray::RayCastResult> results;
		results.reserve(4);
		for (int i = 0; i < 4; ++i) {
			const auto seg = LineSegment(bounds.getVertex(i), bounds.getVertex((i + 1) % 4));
			if (auto r = ray.castLineSegment(seg)) {
				results.push_back(*r);
			}
		}
		if (results.empty()) {
			return origin;
		}

		std::sort(results.begin(), results.end());
		return results.back().pos;
	}
}

void AnimationEditorDisplay::draw(UIPainter& painter) const
{
	painter.draw(drawSprite);
	painter.draw(boundsSprite);
	if (actionPointSprite.isVisible()) {
		painter.draw(actionPointSprite);
	}
	if (nineSliceHSprite.isVisible()) {
		painter.draw(nineSliceHSprite);
	}
	if (nineSliceVSprite.isVisible()) {
		painter.draw(nineSliceVSprite);
	}
	
	const auto mousePos = screenSpaceMousePos;
	const bool iso = isoMode;
	const auto rect = getRect();
	auto p2 = painter.withClip(rect);
	p2.draw([rect, mousePos, iso] (Painter& p)
	{
		const auto colour = Colour4f(1, 0, 1, 0.4f);
		const auto a0 = Angle1f::fromDegrees(iso ? 26.5650512f : 0.0f);
		const auto a1 = Angle1f::fromDegrees(iso ? -26.5650512f : 90.0f);
		const auto a180 = Angle1f::fromDegrees(180);
		const auto p0 = findPoint(mousePos, a0, rect);
		const auto p1 = findPoint(mousePos, a0 + a180, rect);
		const auto p2 = findPoint(mousePos, a1, rect);
		const auto p3 = findPoint(mousePos, a1 + a180, rect);
		const auto off = Vector2f(0.5f, 0.5f);
		
		p.drawLine(LineSegment(p0, p1) + off, 1.0f, colour);
		p.drawLine(LineSegment(p2, p3) + off, 1.0f, colour);
	});
}

void AnimationEditorDisplay::onMouseOver(Vector2f mousePos, KeyMods keyMods)
{
	this->screenSpaceMousePos = mousePos;
	this->mousePos = screenToImageSpace(mousePos);
	isoMode = (keyMods & KeyMods::Ctrl) != KeyMods::None;
}

void AnimationEditorDisplay::setMetadataEditor(MetadataEditor& metadataEditor)
{
	this->metadataEditor = &metadataEditor;
}

bool AnimationEditorDisplay::isPlaying() const
{
	return animationPlayer.getPlaybackSpeed() > 0.01f;
}

void AnimationEditorDisplay::setPlaying(bool play)
{
	animationPlayer.setPlaybackSpeed(play ? 1.0f : 0.0f);
}

void AnimationEditorDisplay::nextFrame()
{
	const auto n = animationPlayer.getCurrentSequenceLength();
	animationPlayer.setTiming(modulo(animationPlayer.getCurrentSequenceFrame() + 1, n), 0);
}

void AnimationEditorDisplay::prevFrame()
{
	const auto n = animationPlayer.getCurrentSequenceLength();
	animationPlayer.setTiming(modulo(animationPlayer.getCurrentSequenceFrame() - 1, n), 0);
}

int AnimationEditorDisplay::getFrameNumber() const
{
	return animationPlayer.getCurrentSequenceFrame();
}

void AnimationEditorDisplay::onDoubleClick()
{
	setCurrentActionPoint(Vector2i(getMousePos()));
}

void AnimationEditorDisplay::setActionPoint(const String& pointId)
{
	actionPointId = pointId;
}

void AnimationEditorDisplay::clearPoint()
{
	setCurrentActionPoint(std::nullopt);
}

void AnimationEditorDisplay::updateBounds()
{
	bounds = Rect4f(origBounds);
	
	setMinSize((bounds.getSize() * zoom).round());
}

Vector2f AnimationEditorDisplay::imageToScreenSpace(Vector2f pos) const
{
	return getPosition() + zoom * pos;
}

Vector2f AnimationEditorDisplay::screenToImageSpace(Vector2f pos) const
{
	return (pos - getPosition()) / zoom;
}

Vector2i AnimationEditorDisplay::getCurrentPivot() const
{
	if (!origPivot) {
		return Vector2i();
	}
	
	return Vector2i(getMetaIntOr("pivotX", origPivot->x), getMetaIntOr("pivotY", origPivot->y));
}

std::optional<Vector2i> AnimationEditorDisplay::getCurrentActionPoint() const
{
	if (actionPointId == "pivot") {
		return getCurrentPivot() - origPivot.value_or(Vector2i());
	}

	const auto actionPoints = metadataEditor->getValue("actionPoints");
	if (actionPoints.hasKey(actionPointId)) {
		const auto& pointConfig = actionPoints[actionPointId];
		const auto& key = animationPlayer.getCurrentSequenceName() + ":" + animationPlayer.getCurrentDirectionName();
		if (pointConfig.hasKey(key)) {
			const auto& seqConfig = pointConfig[key];
			if (seqConfig.asSequence().size() > animationPlayer.getCurrentSequenceFrame()) {
				const auto& value = seqConfig[animationPlayer.getCurrentSequenceFrame()];
				if (value.getType() != ConfigNodeType::Undefined) {
					return value.asVector2i() - origPivot.value_or(Vector2i());
				}
			}
		}

	}

	return {};
}

void AnimationEditorDisplay::setCurrentActionPoint(std::optional<Vector2i> pos)
{
	if (actionPointId == "pivot" && pos) {
		metadataEditor->setPivot(*pos);
		refresh();
		return;
	}

	auto actionPoints = metadataEditor->getValue("actionPoints");
	auto& pointConfig = actionPoints[actionPointId];
	pointConfig.ensureType(ConfigNodeType::Map);
	const auto key = animationPlayer.getCurrentSequenceName() + ":" + animationPlayer.getCurrentDirectionName();
	auto& seqConfig = pointConfig[key];
	seqConfig.ensureType(ConfigNodeType::Sequence);
	if (seqConfig.getSequenceSize() <= animationPlayer.getCurrentSequenceFrame()) {
		seqConfig.asSequence().resize(animationPlayer.getCurrentSequenceFrame() + 1);
	}

	auto& config = seqConfig[animationPlayer.getCurrentSequenceFrame()];
	if (pos) {
		config = *pos;
	} else {
		config = ConfigNode();
		if (std::all_of(seqConfig.asSequence().begin(), seqConfig.asSequence().end(), [](const ConfigNode& e) { return e.getType() == ConfigNodeType::Undefined; })) {
			pointConfig.removeKey(key);
		}
	}

	metadataEditor->setValue("actionPoints", std::move(actionPoints));
}

std::optional<Vector4f> AnimationEditorDisplay::getCurrentSlices() const
{
	const auto s = Vector4i(origSprite.getSlices());
	const auto slices = Vector4i(getMetaIntOr("slice_left", s.x), getMetaIntOr("slice_top", s.y), getMetaIntOr("slice_right", s.z), getMetaIntOr("slice_bottom", s.w));
	if (slices.x == 0 && slices.y == 0 && slices.z == 0 && slices.w == 0) {
		return {};
	}
	return Vector4f(slices);
}

int AnimationEditorDisplay::getMetaIntOr(const String& key, int defaultValue) const
{
	if (!metadataEditor) {
		return 0;
	}
	const auto result = metadataEditor->getString(key);
	if (result.isEmpty() || !result.isInteger()) {
		return defaultValue;
	}
	return result.toInteger();
}
