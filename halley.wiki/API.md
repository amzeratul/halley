# Overview

The **Halley API** is a set of abstract interfaces that represent implementations of several lower level concepts. The following APIs are available:

* [[Core API]] - Controls game flow and resources (always implemented by **halley-core**)
* [[System API]] - File I/O, including loading assets and saving data, as well as some threading support and video display support (platform dependant)
* [[Video API]] - Rendering (platform dependant)
* [[Audio API]] - Audio mixing and rendering (implemented by **halley-audio**)
* [[Audio Output API]] - Audio output (platform dependant)
* [[Input API]] - Input reading (platform dependant)
* [[Network API]] - Networking and sockets (platform dependant)
* [[Movie API]] - Movie playback (platform dependant)
* [[Platform API]] - Platform features, such as authentication, achievements, etc (platform dependant)

For information on how to initialise the API, see [[Game]].

# Dummy APIs

Dummy APIs are provided with the engine. With the exception of System, they are sufficient to allow games to run on an environment where the remaining APIs have not yet been implemented. This allows for quicker iteration on porting to new platforms.

Dummy APIs are always registered, but have lower priority, and should be taken over by any real implementation.