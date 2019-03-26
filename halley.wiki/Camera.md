# Overview
The camera represents a mapping between the world space and screen space. It can be moved, rotated, and zoom in and out. It generates a Model-View-Projection (MVP) matrix, which is then used by Halley to be fed to all [[Materials]], for vertex transformation.

Note that the position of the camera represents its **centre**. A typical camera to render a non-scrolling 1280x720 window might be initialised something like this:

```c++
auto cam = Camera(Vector2f(1280, 720) * 0.5f);
```

To use a camera, it must be bound to a [[Render Context]] before it's used. See render context documentation for more information.

# Methods
## setPosition
```c++
Camera& Camera::setPosition(Vector2f pos)
```
Sets the position of the **centre** of the camera. Returns *this.

## setAngle
```c++
Camera& Camera::setAngle(Angle1f angle)
```
Sets the angle of rotation of the camera. Returns *this.

## setZoom
```c++
Camera& Camera::setZoom(float zoom)
```
Sets the zoom level of the camera. For example, passing 2.0f here will cause everything to look twice as big. Returns *this.

## setViewPort
```c++
Camera& Camera::setViewPort(Rect4i viewPort)
```
Sets the area of the [[Render Target]] that this camera will render to. Useful for split screen and PIP. Coordinates are in screen space, with (0, 0) on top-left. Returns *this.

## resetViewPort
```c++
Camera& Camera::resetViewPort()
```
Sets the camera to cover the entire [[Render Target]] again. Returns *this.