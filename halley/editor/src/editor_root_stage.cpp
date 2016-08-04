#include "editor_root_stage.h"

using namespace Halley;

void EditorRootStage::init()
{
	auto col = Colour4f(0.065f);
	halleyLogo = Sprite()
		.setImage(getResources(), "halley_logo_dist.png", "distance_field_sprite.yaml")
		.setPivot(Vector2f(0.5f, 0.5f))
		.setColour(col)
		.setScale(Vector2f(2, 2))
		.setPos(Vector2f(640, 360));
	auto& mat = halleyLogo.getMaterial();
	mat["u_smoothness"] = 0.1f;
	mat["u_outline"] = 0.0f;
	mat["u_outlineColour"] = col;
}

void EditorRootStage::onVariableUpdate(Time time)
{
	halleyLogo.setPos(Vector2f(getVideoAPI().getWindow().getSize() / 2));
}

void EditorRootStage::onRender(RenderContext& context) const
{
	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0));

		// Background
		auto matRaw = std::make_shared<Material>(getResource<MaterialDefinition>("scanlines.yaml"));
		auto& mat = *matRaw;
		auto sprite = Sprite()
			.setMaterial(matRaw)
			.setPos(Vector2f(0, 0))
			.setTexRect(Rect4f(painter.getViewPort()))
			.setSize(Vector2f(painter.getViewPort().getSize()));
		mat["u_col0"] = Colour4f(0.08f);
		mat["u_col1"] = Colour4f(0.07f);
		mat["u_distance"] = 6.0f;
		sprite.draw(painter);

		halleyLogo.draw(painter);
	});
}
