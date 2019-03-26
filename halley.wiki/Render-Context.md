# Overview

A Render Context (**Halley::RenderContext**) is an abstraction that configures how you're going to perform rendering. It can be passed around, and is immutable, however you can create modified versions of it with the **with()** family of methods. To actually draw with a given context, you can use the **bind()** method.

# Methods

## with (Camera)
```c++
RenderContext RenderContext::with(Camera& camera) const;
```
Returns a copy of the current context, but using the provided [[Camera]] instead. Use this to perform zooming and scrolling, for example.

## with (RenderTarget)
```c++
RenderContext RenderContext::with(RenderTarget& target) const;
```
Returns a copy of the current context, but using the provided [[Render Target]] instead. Use this to render to a texture, for example.

## bind
```c++
void RenderContext::bind(std::function<void(Painter&)> f);
```
Binds the render context, invokes the provided function (which will receive a valid [[Painter]] reference), then unbinds the render context. Use this to perform actual drawing.

Note that you shouldn't use any render contexts inside this callback.

# Example
```c++
void MyStage::onRender(RenderContext& context) const
{
	auto screenCam = Camera(Vector2f(currentResolution) * 0.5f);

	context.with(screenCam).bind([&](Painter& painter)
	{
		painter.clear(Colour(0, 0, 0, 1.0f));
		Sprite()
			.setImage(getResources(), "test.png")
			.setPos(Vector2f(currentResolution) * 0.25f)
			.draw(painter);
	});
}
```