#include "audio_layers_editor.h"

#include "audio_expression_editor.h"
#include "audio_fade_editor.h"
#include "audio_object_editor.h"
#include "halley/audio/sub_objects/audio_sub_object_layers.h"

using namespace Halley;

AudioLayersEditor::AudioLayersEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectLayers& layers)
	: UIWidget("audio_layers_editor", Vector2f(), UISizer())
	, factory(factory)
	, editor(editor)
	, layers(layers)
{
	factory.loadUI(*this, "halley/audio_editor/audio_layers_editor");
}

void AudioLayersEditor::onMakeUI()
{
	getWidget("fadeContainer")->add(std::make_shared<AudioFadeEditor>(factory, layers.getFade(), [=] ()
	{
		editor.markModified(false);
	}));

	const auto layerList = getWidgetAs<UIList>("layers");
	for (size_t i = 0; i < layers.getLayers().size(); ++i) {
		layerList->addItem(toString(i), std::make_shared<AudioLayersEditorLayer>(factory, *this, i), 1);
	}

	bindData("name", layers.getRawName(), [=] (String v)
	{
		editor.markModified(true);
		layers.setName(std::move(v));
	});

	setHandle(UIEventType::ListItemsSwapped, [=] (const UIEvent& event)
	{
		std::swap(layers.getLayers()[event.getIntData()], layers.getLayers()[event.getIntData2()]);
		editor.markModified(true);
		refreshIds();
	});
}

AudioSubObjectLayers::Layer& AudioLayersEditor::getLayer(size_t idx)
{
	return layers.getLayers()[idx];
}

void AudioLayersEditor::markModified(size_t idx)
{
	editor.markModified(false);
}

AudioObjectEditor& AudioLayersEditor::getEditor()
{
	return editor;
}

void AudioLayersEditor::refreshIds()
{
	const auto layerList = getWidgetAs<UIList>("layers");
	const auto n = layerList->getCount();
	for (size_t i = 0; i < n; ++i) {
		const auto layerEditor = layerList->getItem(static_cast<int>(i))->getWidgetAs<AudioLayersEditorLayer>("audio_layers_editor_layer");
		layerEditor->setIdx(i);
	}
}

AudioLayersEditorLayer::AudioLayersEditorLayer(UIFactory& factory, AudioLayersEditor& layersEditor, size_t idx)
	: UIWidget("audio_layers_editor_layer", Vector2f(), UISizer())
	, factory(factory)
	, layersEditor(layersEditor)
	, idx(idx)
{
	factory.loadUI(*this, "halley/audio_editor/audio_layers_editor_layer");
}

void AudioLayersEditorLayer::onMakeUI()
{
	auto& layer = layersEditor.getLayer(idx);
	const auto& name = layer.object->getName();
	getWidgetAs<UILabel>("layerName")->setText(LocalisedString::fromUserString(name));

	getWidget("expressionContainer")->add(std::make_shared<AudioExpressionEditor>(factory, layer.expression, layersEditor.getEditor()), 1);

	bindData("synchronised", layer.synchronised, [this, &layer] (bool value)
	{
		layer.synchronised = value;
		layersEditor.markModified(idx);
	});

	bindData("restartFromBeginning", layer.restartFromBeginning, [this, &layer] (bool value)
	{
		layer.restartFromBeginning = value;
		layersEditor.markModified(idx);
	});

	bindData("onlyFadeInWhenResuming", layer.onlyFadeInWhenResuming, [this, &layer] (bool value)
	{
		layer.onlyFadeInWhenResuming = value;
		layersEditor.markModified(idx);
	});

	bindData("delay", layer.delay, [this, &layer] (float value)
	{
		layer.delay = value;
		layersEditor.markModified(idx);
	});

	auto updateFadeOverride = [=](bool value, std::optional<AudioFade>& fade, const char* containerId)
	{
		if (!!fade != value) {
			fade = value ? AudioFade() : std::optional<AudioFade>();
		}

		auto container = getWidget(containerId);
		container->setActive(value);
		if (value) {
			container->add(std::make_shared<AudioFadeEditor>(factory, *fade, [this] ()
			{
				layersEditor.markModified(idx);
			}));
		} else {
			container->clear();
		}
	};

	bindData("overrideFadeIn", !!layer.fadeIn, [=, &layer] (bool value)
	{
		updateFadeOverride(value, layer.fadeIn, "fadeInContainer");		
		layersEditor.markModified(idx);
	});

	bindData("overrideFadeOut", !!layer.fadeOut, [=, &layer] (bool value)
	{
		updateFadeOverride(value, layer.fadeOut, "fadeOutContainer");		
		layersEditor.markModified(idx);
	});

	updateFadeOverride(!!layer.fadeIn, layer.fadeIn, "fadeInContainer");
	updateFadeOverride(!!layer.fadeOut, layer.fadeOut, "fadeOutContainer");
}

void AudioLayersEditorLayer::setIdx(size_t idx)
{
	this->idx = idx;
}
