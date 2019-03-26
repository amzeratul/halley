# Overview
Entities are abstract objects, that possess only an **EntityId**, and can contain [[Components]]. They can only have one copy of each type of component, and they do not possess any logic of their own; relying entirely on [[Systems]] to manipulate them.

They can be created and destroyed from the [[World]]. Typically, instead of manipulating **Halley::Entity** objects directly, you'll be operating with **Halley::EntityRef**, which provides an entry point for manipulating entities.

# Methods
## addComponent
```c++
template <typename T> EntityRef& EntityRef::addComponent(T&& component)
```
Adds a [[component|Components]] to an entity. This makes this entity be included in the families of any [[systems]] whose requirements it now fulfils. An entity can only have one component of each type. See [[Components]] for more detailed information on components, or [[Systems]] for more information on families. Returns *this.

This method can be chained, e.g.:

```c++
world->createEntity()
	.addComponent(PositionComponent(Vector2f()))
	.addComponent(CameraComponent(Camera(), 0, int(SpriteMasks::InWorld), Colour4f(0, 0, 0, 1), {}))
	.addComponent(HierarchyComponent({}, camRoot.getEntityId(), {}, true, false));
```

## removeComponent
```c++
template <typename T> EntityRef& EntityRef::removeComponent()
```
Removes a [[component|Components]] from an entity. This will cause it to be removed from any families which require that component. Returns *this.

## getComponent
```c++
template <typename T> T* EntityRef::getComponent()
```
Returns a pointer to the specified component, or **nullptr** if this entity does not contain it.

## getEntityId
```c++
EntityId EntityRef::getEntityId() const
```
Returns the **Halley::EntityId** of this entity. This is unique per world, and can be used to later reference this entity.