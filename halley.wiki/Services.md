# Overview
Services are user-defined classes that [[Systems]] can access. They can be used as "loopholes" around the [[ECS|ECS Overview]] system (e.g. to allow systems to talk directly to each other), or to allow systems to access game data which is stored outside the ECS.

# Declaration
Services are declared in YAML like other custom types:

```c++
type:
  name: InputService
  include: "src/services/input_service.h"
```

# Implementation
Implementation of services is entirely up to the user. The only requirement is that they must extend `Halley::Service`. They can then be accessed by systems by declaring the system as needing that service, see [[Systems]] for more details.