# Overview

The Painter (**Halley::Painter**) executes the drawing on the currently bound [[Render Target]], using the currently bound [[Camera]]. It can't be instantiated by the user, and must always be obtained via the [[Render Context]]. Painter is a virtual class that is implemented by the [[Video API]].

Most of the time, you won't use most of the methods on Painter, instead relying on [[Sprites]], [[Text Rendering]] and the [[Sprite Painter]]. However, a few methods (such as **clear**) are useful.

The painter provides dynamic batching of vertices and sprites. When you submit data for the painter, instead of drawing it immediately, it is held in a queue. Subsequent data, if compatible with the existing data (typically meaning having identical [[materials|Materials]]) will be appended to the queue. If the data is not compatible, existing data is flushed, a new queue is started, and the process continues. This allows Halley to drastically reduce the number of draw calls, without user interaction.

# Methods
## clear
```c++
virtual void Painter::clear(Colour colour) = 0
```
Clears the target to the selected colour. Useful at the start of rendering.

## flush
```c++
void Painter::flush()
```
Forces the rendering of any pending vertices.