# Overview
Sprites are the main way in which images are drawn in Halley. Each sprite represents a quad (drawn as two triangles), which can be positioned, rotated, scaled, coloured and texture mapped. A sprite also contains a reference to a [[Material|Materials]]  that will be used to draw it, and it might contain clipping and slicing information.

[[Animation]] and [[Text Rendering]] generate sprites for rendering.

Sprites are can be drawn directly to a [[Painter]] (see **draw**, below), or they can be queued in a [[Sprite Painter]] for drawing with sorting.

# Initialising sprites
Sprites are typically constructed empty, then modified with accessors. The most typical way to initialise one is by providing an image name, and, optionally, a [[Material]] name (`Halley/Sprite` will be used by default).

```c++
auto s1 = Sprite().setImage(resources, "mySprite.png");
auto s2 = Sprite().setImage(resources, "light.png", "Halley/SpriteAdd");
```

You can also pass explicit textures and material definitions, like so:

```c++
auto s3 = Sprite().setImage(resources.get<Texture>("mySprite.png"), resources.get<MaterialDefinition>("Halley/SpriteAdd"));
```

If the sprite is being loaded from a spritesheet, it's also possible to load them with the setSprite method:

```c++
auto s4 = Sprite().setSprite(resources, "lights", "myCoolLight", "Halley/SpriteAdd");
auto s5 = Sprite().setSprite(resources.get<SpriteSheet>("lights"), "myCoolLight");
auto s6 = Sprite().setSprite(resources.get<SpriteSheet>("lights")->getSprite("myCoolLight"));
```

# Drawing sprites
To draw a sprite, simply call the **draw** method with the painter as a parameter:

```c++
Sprite()
	.setImage(resources, "mySprite.png")
	.setPosition(Vector2f(100, 50))
	.draw(painter);
```

You can also use the **drawSliced** method and pass a **Halley::Vector4s** representing the slices, to draw a non-sliced sprite with slices. Typically, though, you'd just set the sprite to be sliced. See below.

# Transforming sprites
Sprites can be **positioned** by calling **setPos** or **setPosition**. This sets the position of their pivot (see below).

```c++
auto s = Sprite()
	.setSprite(resources, "mySprite.png")
	.setPos(Vector2f(50.0f, 30.0f));
```

The **pivot** can be set in relative terms by using **setPivot**, or in absolute terms using **setPivotAbsolute**. When using relative, **(0, 0)** refers to the top-left corner of the sprite and **(1, 1)** refers to the bottom-right corner of the sprite.

```c++
auto s = Sprite()
	.setSprite(resources, "mySprite.png")
	.setPos(Vector2f(50.0f, 30.0f))
	.setPivot(Vector2f(0.5f, 0.5f)); // The pivot is now on the sprite's centre
```

The sprite can be **rotated** around by using **setRotation**. It always rotates around its pivot.

```c++
auto s = Sprite()
	.setSprite(resources, "mySprite.png")
	.setPos(Vector2f(50.0f, 30.0f))
	.setPivot(Vector2f(0.5f, 0.5f))
	.setRotation(Angle1f::fromDegrees(45.0f)); // Sprite is now rotated 45° around its pivot
```

**Size** and **scale** are two related, but distinct concepts. The size indicates the actual physical size of the sprite (typically, the same size as the cropped image), and the scale indicates the zoom level. This can make an important difference when using sliced sprites - you typically want to adjust their scale, not their size. **setSize** and **setScale** let you control those values directly. A convenience method called **scaleTo** is also provided, that computes the scale factor necessary for the sprite to reach a certain size.

```c++
Sprite()
	.setImage(resources, "my32x32image.png")
	.scaleTo(Vector2f(128.0f, 64.0f)); // Scale is now Vector2f(4.0f, 2.0f)
```

Sprites can also be **flipped** horizontally around its pivot with **setFlip**. This is a binary flag. If you want to flip vertically, you can flip it horizontally, then rotate it by 180 degrees.

```c++
auto s = Sprite()
	.setSprite(resources, "mySprite.png")
	.setFlip(true);
```

# Pivots
The pivot is the "fixed point" of the sprite. It's the point that is at the actual position specified by the sprite. When rotating, scaling and flipping, the pivot does not move; all other points move AROUND it.

For example, if you have a sprite representing a rotating wheel, you might want to make sure the pivot is on the centre of the wheel, so it rotates around that point. Or, if you have a character in a platformer, you might want the pivot in the centre between the character's feet.

The pivot can be read automatically from images (via metadata) and spritesheets. See [[Images and Textures]].

# Colour, texture and material
The **colour** of the sprite can be set with **setColour**. The default colour is opaque white (`Colour4f(1, 1, 1, 1)`). This will be passed on to the pixel shader to do as it pleases with it. The implementation from `Halley/Sprite` will modulate the sprite's colours with the colour passed, so the colour can be used to tint the sprite or fade it out with alpha. Colour values above 1 are supported, and Halley's shaders will interpret that as a value to add on top of the colour (causing brightening).

You can manually specify the texture rectangle that this sprite represents by calling **setTexRect**. The coordinates are in (0, 0) to (1, 1) format, with (0, 0) at the top-left.

It's also possible to manually set a sprite's material by calling **setMaterial**.

A sprite can be set to be invisible by calling **setVisible**. Invisible sprites don't get drawn, and, in fact, don't even get sent to the painter.

# Sliced sprites
A sprite can be 9-sliced. This is typically done via the asset's metafile (see [[Images and Textures]]). The 9-slice is defined by four border values, which is the distance from the edge of the image on left, top, right, bottom. This is typically represented as a **Halley::Vector4s**, with those four values in that order.

It's called a 9-slice because the sprite is sliced in 9 sprites, like in a `#` sign.

When drawing a sliced sprite, the centre piece resizes normally, the corner pieces don't resize at all, and the edge pieces only resize along the centre. This is very useful when drawing UI boxes, for example. If the sprite is loaded from a sliced image, it will automatically load slicing information and be drawn as sliced. You can manually specify the slicing of a sprite by calling **setSliced**, or undo it by calling **setNotSliced**.

Note: sliced sprites are not currently batched together.

# Borders
Halley's asset importer will trim empty space around an image by default, even if it was supposed to be part of the sprite. Although this behaviour can be disabled (see [[Images and Textures]]), you sometimes want to keep it (for the performance gains), but still know the original size. The **getOuterBorder** method returns a **Halley::Vector4s** that tells you how much border was trimmed from, in order, left, top, right, bottom.

# Clipping
A sprite can be drawn with clipping on. It can be set or removed with **setClip**. Clipping is relative to the pivot of the sprite, so, for example:

```c++
Sprite()
	.setImage(resources, "my32x32image.png")
	.setPivot(Vector2f(0.5f, 0.5f))
	.setClip(Rect4f(0, 0, 16, 16));  // Only shows the bottom-right of the sprite
```

Clipped sprites cannot be batched.