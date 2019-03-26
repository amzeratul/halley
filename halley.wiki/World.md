# Overview
The world (**Halley::World**) is the top-level object that stores data for the [[Entity Component System|ECS overview]].

The world represents the set of [[entities|Entities]] that can see and interact with each other, plus the [[systems|Systems]] that can manipulate them, and the [[services|Services]] they can access. You typically have only one world in a given [[stage|Stages]], but you could have more (if, for example, your game is simulating multiple independent "worlds").

# Declaring a world in a YAML config file
Worlds are normally declared as a YAML, which specifies which [[Systems]] run, in which order, and in which [[timelines|Main Loop]]. This YAML can then be picked up by the **createWorld** method of **Halley::EntityStage** (see [[Stages]]). An example file looks like this:

```yaml
---
timelines:
  variableUpdate:
    - TitleScreenUnitSpawn

    - Move
    - Hierarchy
    - LoopingSound
    - Time

    - PlayerColour
    - DestroyAfterAnimation
    - SpriteAnimation
    - TimeShader

  render:
    - SpriteRender
    - UIShadowRender
    - CameraRender
...
```

# Methods
## Constructor
```c++
World::World(const HalleyAPI* api);
```
Creates a new world. Requires a pointer to the Halley [[API]]. You might instead want to create it using the **Halley::EntityStage** (see [[Stages]]).

## addSystem
```c++
System& World::addSystem(std::unique_ptr<System> system, TimeLine timeline);
```
Manually adds a [[system|Systems]] to the world. You'd typically load the systems from a config file, instead (see [[Stages]]).

## addService
```c++
Service& World::addService(std::shared_ptr<Service> service);
```
Adds a [[service|Services]] to the world. You must manually add any services required by any of the systems before stepping the world for the first time.

## createEntity
```c++
EntityRef World::createEntity();
```
Creates a new, empty [[entity|Entities]]. The returned **Halley::EntityRef** allows you to add components to it by chaining **.addComponent** on it. For example:

```c++
// Create the game camera
world->createEntity()
	.addComponent(PositionComponent(Vector2f()))
	.addComponent(CameraComponent(Camera(), 0, int(SpriteMasks::InWorld), Colour4f(0, 0, 0, 1), {}))
	.addComponent(HierarchyComponent({}, camRoot.getEntityId(), {}, true, false));
```

## destroyEntity
```c++
void World::destroyEntity(EntityId id);
```
Destroys the [[entity|Entities]] with the given **Halley::EntityId**. Does nothing if the entity doesn't exist.

## getEntity
```c++
EntityRef World::getEntity(EntityId id);
```
Returns a **Halley::EntityRef** to the [[entity|Entities]] with **Halley::EntityId**. Throws an exception if it doesn't exist. Note that this method is considered a "loophole" around the entity system, and should not normally be used.

## tryGetEntity
```c++
Entity* tryGetEntity(EntityId id);
```
Obtains a pointer to the actual [[entity|Entities]], based on its **Halley::EntityId**. Returns **nullptr** if it doesn't exist. Note that this method is a "loophole" on the entity system, and should not normally be used.

## step
```c++
void World::step(TimeLine timeline, Time elapsed);
```
Steps the world on one of the [[timelines|Main Loop]]. Typically called from **onVariableUpdate** and **onFixedUpdate**. This is where most of the world logic actually runs.

## render
```c++
void World::render(RenderContext& rc) const;
```
Rendering analogue to **step**. This is where all render systems have a chance to execute.