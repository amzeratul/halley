== Halley 3.x.x ==

=== Breaking Changes ===
* `Game::startGame()` no longer takes API as a parameter. Use `Game::getAPI()` instead.
* `Halley::Maybe<T>` is now deprecated. Use `std::optional<T>` instead.
* `EntityStage::createWorld()` no longer takes `createComponent` and `createSystem` arguments.
* Removed `Sprite::getOriginalSize()`. Use `Sprite::getUncroppedSize()` instead.
* Removed `Sprite::getRawSize()`. Use `Sprite::getSize()` instead.
* Removed `Halley::AABB`. Use `Halley::Rect4f` instead.
* `UIRoot` constructor changed, takes const `Halley::HalleyAPI&` instead of `const AudioAPI*`.
* `UITextInput` constructor changed, no longer takes `std::shared_ptr<InputKeyboard>`.
* `UISpinControl` constructor changed, no longer takes `std::shared_ptr<InputKeyboard>`.
* `UIWidget` can no longer be focused by calling `UIWidget::setFocus(true)`. Use `UIRoot::setFocus(widget)` instead.
