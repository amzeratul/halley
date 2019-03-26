# Overview
The Halley Main Loop is executed after engine initialisation, and runs until **Halley::CoreAPI::quit** is called. It updates the three timelines in accordance to the framerate set in **Halley::Game::getTargetFPS**.

# Timelines
There are three timelines: **fixed**, **variable** and **render**. Each of those three have a corresponding method on the current [[stage||Stages]], which will be called every time the main loop ticks the matching timeline. Most games can get away with ignoring the Fixed timeline, and only use Variable timeline for updating game logic, then Render timeline to draw the screen.

## Fixed
The fixed timeline is run, if possible, at the target FPS specified. It might run more or less than once per variable update. A fixed time slice (1/FPS) will be passed on every invocation of onFixedUpdate.

## Variable
The variable timeline is run once before each render tick. It's the recommended timeline for performing any updates that don't become unstable at low FPS, since it'll be more closely matched to what's rendered on the screen. The time since the last update will be passed on every invocation of onVariableUpdate.

## Render
The render timeline is run every time the screen needs to be updated. Each tick on this timeline is guaranteed to be preceded by a tick in the variable timeline. This timeline should NOT cause any change in state, and it shouldn't be aware of time elapsed.