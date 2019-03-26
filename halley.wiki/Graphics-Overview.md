# Overview

Graphics rendering in Halley happens via a [[Render Context]]. Once bound (see its documentation for more information), you'll get access to a [[Painter]], which can be used to render on the current target. By default, the target is the screen, but the render context can be re-bound to a texture (see [[Render Target]]).

Though the [[Painter]] provides lower level rendering routines, you'll typically use [[Sprites]] and [[Text Renderers|Text Rendering]] instead. You can use them directly, or you can sort them in layers using a [[Sprite Painter]].

Sprites and Text are drawn using [[Materials]], which encapsulate the rendering technique (shaders, etc) used to draw them.

[[Animation]] is also supported.