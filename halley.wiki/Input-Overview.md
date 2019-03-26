# Overview

Input in Halley is generally handed by the **Halley::InputDevice** interface. You can obtain handles to the different input devices by calling the appropriate methods in the [[Input API]].

The typical use case involves creating one [[Virtual Input]] device, then binding all relevant inputs to it, and using that virtual device throughout the code base.