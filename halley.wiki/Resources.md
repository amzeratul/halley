# Overview
Data files that are accessed by the game are called **Resources**. They are typically loaded from physical files on the hard drive, called **assets**, which reside under the **/assets** folder, and are imported by either **halley-cmd** or **halley-editor** (see [[Importing Assets]]).

# Types of resources
Halley comes with out-of-the-box support for the following resource types:

* **Texture**: Images loaded as textures to the video card. _Should be stored under /assets_src/image_
* **SpriteSheet**: A collection of sprites within a texture (see [[Sprites]]). _Should be stored under /assets_src/spritesheet_
* **SpriteResource**: A resource representing a single sprite in a texture (see [[Sprites]]). _Should be stored under /assets_src/image_
* **Animation**: Animation definition, used to animate sprites (see [[Animation]]). _Should be stored under /assets_src/animation_
* **MaterialDefinition**: Material definition, used to define how Sprites are drawn (see [[Materials]]). _Should be stored under /assets_src/material_
* **ShaderFile**: A file containing shader data, used by Materials. _Should be stored under /assets_src/shader_
* **BinaryFile**: A general-purpose binary file. _Should be stored somewhere inside /assets_src_
* **TextFile**: A general-purpose ASCII or UTF-8 text file. _Should be stored somewhere inside /assets_src_
* **ConfigFile**: A configuration file, read as YAML and imported as binary (see [[Config Files]]). _Should be stored under /assets_src/config_
* **AudioClip**: A sound effect or music track (see [[Audio Clips]]). _Should be stored under /assets_src/audio_
* **Font**: A vector or bitmap font, loaded as a vector field (see [[Fonts]]). _Should be stored under /assets_src/font_

# Asset metafiles
Asset files can have an accompanying metafile. The name of the metafile should be `origFileName.origExt.meta`, and it should sit side-by-side with the original file. For example, if you have a `myImage.png`, its corresponding metafile should be `myImage.png.meta`.

Metafiles are **YAML** files. They can contain additional information relevant to each type of asset. See the relevant documentation page for more information on the types of meta data that can be stored for each format.

# The Halley::Resources class
Resources are loaded by specifying their name and type via the **Halley::Resources** class, which is accessible from [[Stages]]. You will typically need to pass this class down to anything that needs to load resources. It's recommended that you do **not** store this as a static variable for convenience.

## get
```c++
template <typename T> std::shared_ptr<const T> Resources::get(const String& name, ResourceLoadPriority priority = ResourceLoadPriority::Normal);
```

Returns a **std::shared_ptr** to a **const** instance of the resource you requested. If the resource has not yet been loaded, it's first loaded. Loaded resources are cached until unloaded. This will block for non-asynchronous resources. If the resource doesn't exist, an exception will be thrown. This method is thread-safe.

For example:

```c++
std::shared_ptr<const MaterialDefinition> matDef = resources.get<MaterialDefinition>("Halley/Sprite");
```

## unload
```c++
template <typename T> std::shared_ptr<const T> Resources::unload(const String& name);
```

Unloads a resource with a given name.

## exists
```c++
template <typename T> bool Resources::exists(const String& name);
```

Returns whether a resource with that name exists.

## enumerate
```c++
template <typename T> std::vector<String> Resources::enumerate();
```

Returns a list of all resources available with a given type.

# Asynchronous Resources
Some resource types are asynchronous, meaning that they will be returned while they're still loading. The engine will ensure consistency; the user does not need to worry about this.

Textures and AudioClips are two examples of asynchronous resources. For textures, if the engine needs to actually bind the texture for drawing, it will stop and wait for it to be loaded. For audio clips, it will delay playback until it's available.

# User-defined Resource Types
It's also possible to define your own resource types. Resources must extend **Halley::Resource** (or **Halley::AsyncResource**). **Resources::init** will also have to be called with your resource type, and a proper static loader method must be available, like the one for existing resource types:

```c++
static std::unique_ptr<MyResource> MyResource::loadResource(ResourceLoader& loader);
```