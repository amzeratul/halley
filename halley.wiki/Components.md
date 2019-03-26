# Overview
Components are what allow [[Entities]] to be different from one another. They store the data associated with each entity, and grant them properties by making them visible to families (see [[Systems]]).

Components should normally be fine grained. For example, if you want a moving sprite on the screen, consider having three components representing that: a **PositionComponent**, a **VelocityComponent**, and a **SpriteComponent**.

# Declaring Components
## Syntax
Components are code generated (see [[ECS Overview]]). They follow the following format:

```yaml
component:
  name: TextLabel
  members:
    - text: 'Halley::TextRenderer'
    - layer: int
    - mask: int
```

At top level, the YAML document should contain an entry called `component`. It should have two keys inside it, `name`, specifying a unique name for the component (NB: the actual class generated will have `Component` appended to it, so in the example above it would generate a class called `TextLabelComponent`), and `members`, which is a list containing all members in the component.

Each member is specified by a name, followed by its C++ type. Note that it's necessary to use the full namespace for Halley classes, as the components will be compiled in translation units that don't invoke `using namespace Halley;`

At this time, it's not possible to specify default values, and all component members must be explicitly initialised in the generated constructor.

## Custom types
If any of your members uses a type that your game declares, you need to help the codegen find the appropriate header to include. You can do this by using a declaration in this format:

```yaml
type:
  name: CurveType
  include: "src/config/art/curve_type.h"
```

Note that the name must match exactly. e.g. if your component needs a `std::vector<CurveType>`, you'd need to declare a type for `name: 'std::vector<CurveType>'`.