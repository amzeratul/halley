# Overview
Materials describe how a particular draw call (including [[Sprites]] and [[Text Rendering]]) should look like. They store the following information:

* The vertex attribute layout
* The textures/samplers used
* The uniforms/constants used
* The drawing passes, and, for each of them:
  * The blend mode used
  * The vertex, geometry and pixel shaders used, for each language supported

Materials may inherit from other materials, copying their original properties and overriding them. Most materials inherit from `Halley/SpriteBase`, since that defines the standard vertex attribute layout, which, in turn, inherits from `Halley/MaterialBase`, which allows Halley to inject its MVP matrix into the shader.

A few default materials are provided by Halley, such as:

* `Halley/MaterialBase`: Base material that describes the Halley constant block, used for the MVP matrix
* `Halley/SpriteBase`: Base material that describes the standard vertex attribute layout used by Halley [[Sprites]]
* `Halley/Sprite`: Standard alpha blended sprite
* `Halley/SpriteAdd`: Standard add blended sprite
* `Halley/SpriteOpaque`: Standard opaque blended sprite
* `Halley/SolidColour`: Draws a solid colour, without textures
* `Halley/DistanceFieldSprite`: Renders images encoded with distance fields
* `Halley/Text`: Renders distance field encoded text, with outlines

# Using MaterialDefinitions and Materials

Materials are usually bound when creating [[Sprites]]. If unspecified, sprites will typically default to `Halley/Sprite` material. Sprites will load their texture image into the first texture defined in the material.

```c++
Sprite sprite = Sprite().setImage(getResources(), "ui/border/mapeditor_bg", "Halley/SpriteOpaque");
```

The material defined in the YAML can be accessed via the [[Resources]] system by specifying its name and requesting type **Halley::MaterialDefinition**. This definition can be used to instantiate a **Halley::Material**, to which you can then set textures and/or uniforms:

```c++
auto mat = std::make_shared<Material>(getResource<MaterialDefinition>("Halley/Scanlines"));
mat
	->set("u_col0", Colour4f(0.08f))
	.set("u_col1", Colour4f(0.07f))
	.set("u_distance", 6.0f);
background = Sprite().setMaterial(mat);
```

It's also possible to initialise the material via the sprite, and then access it to modify it:

```c++
background = Sprite()
	.setImage(resources, "round_rect.png", "Halley/DistanceFieldSprite")
	.setColour(Colour4f(0.0f, 0.0f, 0.0f, 0.4f))
	.setPivot(Vector2f(0, 0));

background.getMaterial()
	.set("tex0", resources.get<Texture>("round_rect.png"))
	.set("u_smoothness", 1.0f / 16.0f)
	.set("u_outline", 0.5f)
	.set("u_outlineColour", Colour(0.47f, 0.47f, 0.47f));
```

# Declaration
Here's an example material, `Halley/Sprite`, which ships with Halley:

```yaml
---
name: Halley/Sprite
base: sprite_base.yaml
textures:
  - tex0: sampler2D
passes:
  - blend: AlphaPremultiplied
    shader:
      - language: glsl
        vertex: sprite.vertex.glsl
        pixel: sprite.pixel.glsl
      - language: hlsl
        vertex: sprite.vertex.hlsl
        pixel: sprite.pixel.hlsl
...
```

Here's an example that uses multi-texturing and uniforms:

```yaml
---
name: Wargroove/PaletteSwap
base: sprite_base.yaml
textures:
  - tex0: sampler2D
  - tex1: sampler2D
uniforms:
  - MaterialBlock:
    - playerColour: float
    - skinColour: float
passes:
  - blend: AlphaPremultiplied
    shader:
      - language: glsl
        vertex: sprite.vertex.glsl
        pixel: palette_swap.pixel.glsl
      - language: hlsl
        vertex: sprite.vertex.hlsl
        pixel: palette_swap.pixel.hlsl
...
```

The `name` field specifies the name of the material. You'll need to use this name when setting up [[Sprites]] or loading the **MaterialDefinition** from the [[Resources]], for example.

The `base` field specifies which material should be used as the base. Typically, this will be `Halley/SpriteBase`.

The `attributes` field specifies the vertex attribute layout. See below.

The `textures` field specifies the names and sampler type of textures. Currently only `sampler2D` is supported. The name of the texture should match the name of the uniform sampler2D in GLSL, but for HLSL, the ORDER of the textures is what matters - the first texture will be bound to register t0 (and sampler s0), and so on. For example, for the second example above:

```glsl
// GLSL
uniform sampler2D tex0;
uniform sampler2D tex1;
```

```hlsl
// HLSL
Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);
SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);
```

The `uniforms` field allows you to list blocks of uniforms. See below.

The `passes` field allows you to list your passes. Most materials only have one pass, but they can have multiple passes. For each draw call, passes are run in order.

## Pass properties

Each pass has the following properties:

`blend` specifies which blend type to use. The following blend modes are supported:
* `Opaque`: Does not perform any blending or alpha masking (default)
* `Alpha`: Performs standard alpha blending
* `AlphaPremultiplied`: Performs premultiplied alpha blending (recommended for sprites)
* `Add`: Performs add blend

`shader` allows you to list groups of shaders. Each group of shader has the following properties:
* `language`: Should be `hlsl` or `glsl`
* `vertex`: Specify the vertex shader file
* `geometry`: Specify the geometry shader file (optional)
* `pixel`: Specify the pixel/fragment shader file

All shader files should be stored under `/assets_src/shader/`.

# Using custom uniforms/constants

The `uniforms` field of the material lists the constant blocks available on all stages of the pipeline. Note that Halley does not support uniforms outside of a block, since that behaviour is not supported in all platforms.

`Halley/MaterialBase` defines the HalleyBlock material, which should always be the first block in any material used by the engine. It's used to fill the MVP (Model-View-Projection) matrix in an efficient way.

```yaml
---
name: Halley/MaterialBase
uniforms:
  - HalleyBlock:
    - u_mvp: mat4
...
```

Inside each uniform block declaration, follows a list of the uniform names and their types. The types must be one of the following:
* `float`: a single float
* `vec2`: a (float, float) vector
* `vec3`: a (float, float, float) vector
* `vec4`: a (float, float, float, float) vector
* `mat4`: a 4x4 float matrix

For GLSL, the material block is resolved by name. It must be declared with `layout(std140)`.

```glsl
layout(std140) uniform HalleyBlock {
	mat4 u_mvp;
};
```

For HLSL, the material block is resolved by address. The first one will be on register **b0**, and so on.

```hlsl
cbuffer HalleyBlock : register(b0) {
    float4x4 u_mvp;
};
```

This means that if you are writing your own custom shader, and you declare a single uniform block, but you still derive from `Halley/MaterialBase` (including via deriving from `Halley/SpriteBase`), then your block will be at address b1:

```yaml
---
name: Halley/DistanceFieldSprite
base: sprite_base.yaml
textures:
  - tex0: sampler2D
uniforms:
  - MaterialBlock:
    - u_smoothness: float
    - u_outline: float
    - u_outlineColour: vec4
passes:
  - blend: Alpha
    shader:
      - language: glsl
        vertex: sprite.vertex.glsl
        pixel: distance_field_sprite.pixel.glsl
      - language: hlsl
        vertex: sprite.vertex.hlsl
        pixel: distance_field_sprite.pixel.hlsl
...
```

```hlsl
cbuffer MaterialBlock : register(b1) {
	float u_smoothness;
	float u_outline;
	float4 u_outlineColour;
};
```

Regardless of which block a uniform appears on, their names must be unique; that's because when invoking **Halley::Material**'s **set** method to set its value, it does not take the block into account. All values must be initialised before being used, failing to do so might result in junk data being sent to the shader.

# Using custom vertex attribute layouts

The `attributes` field of the material lists the attributes and their types. The order and sizes must match the data fed to the vertex buffer, typically from **Halley::SpriteVertexAttrib**. To this end, most Halley materials derive from `Halley/SpriteBase`. However, if you're feeding vertices with a different layout, you should also define your own attributes.

The base sprite material:

```yaml
---
name: Halley/SpriteBase
base: material_base.yaml
attributes:
  - a_vertPos: vec4          # xy = relative position of vertex [0..1], zw = relative position of texture [0..1]
  - a_position: vec2         # position (world space)
  - a_pivot: vec2            # relative pivot [0..1]
  - a_size: vec2             # size (px), should be the size of the texture
  - a_scale: vec2            # scale (relative)
  - a_colour: vec4           # rgba
  - a_texCoord0: vec4        # xy = top-left, zw = bottom-right
  - a_rotation: float        # rotation (radians)
  - a_textureRotation: float # is the sprite rotated? (1 if 90 degrees rotated)
...
```

Valid attribute types are:
* `float`: a single float
* `vec2`: a (float, float) vector
* `vec3`: a (float, float, float) vector
* `vec4`: a (float, float, float, float) vector
* `mat4`: a 4x4 float matrix

Note how the material definition above matches **Halley::SpriteVertexAttrib**:

```c++
struct SpriteVertexAttrib
{
	// This structure must match the layout of the shader
	// See shared_assets/material/sprite_base.yaml for reference
	Vector4f vertPos;
	Vector2f pos;
	Vector2f pivot;
	Vector2f size;
	Vector2f scale;
	Colour4f colour;
	Rect4f texRect;
	float rotation = 0;
	float textureRotation = 0;
	char _padding[8];
};
```

In GLSL, the attributes are passed to the vertex shader by name:

```glsl
in vec4 a_vertPos;
in vec2 a_position;
in vec2 a_pivot;
in vec2 a_size;
in vec2 a_scale;
in vec4 a_colour;
in vec4 a_texCoord0;
in float a_rotation;
in float a_textureRotation;
```

However, in HLSL, the name is stripped of its prefix, converted to uppercase, and used as the name of the semantics:

```hlsl
struct VIn {
    float4 vertPos : VERTPOS;
    float2 position : POSITION;
    float2 pivot : PIVOT;
    float2 size : SIZE;
    float2 scale : SCALE;
    float4 colour : COLOUR;
    float4 texCoord0 : TEXCOORD0;
    float rotation : ROTATION;
    float textureRotation : TEXTUREROTATION;
};
```

Because of this, you always need an attribute named `a_position`, so HLSL gets its mandatory `POSITION` semantic.