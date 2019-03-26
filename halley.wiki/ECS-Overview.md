# Overview

Halley uses a "pure" Entity Component System (ECS) for handling entities in games. Usage of the ECS is optional. Components and systems are specified in YAML files, and are code generated during the [[Import Assets|Importing Assets]] stage. Services are also declared in YAML files.

There are six relevant classes of object featured in the ECS:
* [[World]]: The world represents a set of entities that can see and interact with each other, plus the systems that can manipulate them. You typically have only one world, but you could have more.
* [[Entities]]: An entity is an aggregate of components, and represents one "object" in the world.
* [[Components]]: A component represents an aspect that an entity can have, along with its associated data. For example, an entity can have a "Position" component and a "Velocity" component.
* [[Systems]]: Systems manipulate entities, since neither entities nor components have code. They operate upon families, which are defined by the set of every component that matches the requirements of a family. Each system is bound to one of the timelines.
* [[Services]]: Services are classes outside of the ECS, that can be queried by systems. They allow systems to access external information, and to communicate with each other.
* [[Messages]]: Messages are small objects that a system can send with a target entity, and that can be received by another system.

# Code generation

The Halley ECS uses code generation to generate [[Components]], [[Systems]], and [[Messages]], and to declare [[Services]] and custom Types.

To declare those types for code generation, the declarations must be written inside .yaml files, in the `/repo_root/gen_src` directory. The name of the YAML file doesn't matter. The codegen process will read every document inside every .yaml file, and use them together to generate the files. All generated files go into `/repo_root/gen`.

The general format for those YAML files is that they are multi-document files, each document containing a single entry, whose top level name defines the type. For example:

```yaml
---

message:
  name: PlayAnimationOnce
  members:
    - sequence: 'Halley::String'

---

system:
  name: SpriteAnimation
  families:
    - main:
      - Sprite: write
      - SpriteAnimation: write
  messages:
    - PlayAnimation: receive
    - PlayAnimationOnce: receive

---

type:
  name: CurveType
  include: "src/config/art/curve_type.h"

---

component:
  name: Move
  members:
    - moving: bool
    - curTime: float
    - totalTime: float
    - start: 'Halley::Vector2f'
    - end: 'Halley::Vector2f'
    - curveType: CurveType

...
```

For the details on the syntax of each specific type, see their documentation pages.