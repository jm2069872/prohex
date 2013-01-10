# ProHex

ProHex Open Firmware for the HexBright FLEX LED flashlight



## Features

ProHex implements a powerful, versatile feature set for the HexBright FLEX. It is in development but currently features:

* 3 presets with smooth fading transitions
* blink mode

Additional modes and features are in planning and will be added periodically. We will be putting together an email list for updates, or you can just watch the GitHub project.



## Using ProHex

From the off state, click to cycle through low, medium, and high just like in the factory HexBright firmware. However, from high, another click will go back to low. We did this because often, turning the brightness down while going through the off mode is more jarring and can be dangerous in certain situations where maintaining vision is important. A press and hold can turn the light off from any brightness level, eliminating the need to go through the high mode to turn the light off from low or medium.

From the off state, a press and hold enables blink mode. Default settings are lower to help increase runtime on blink mode, allowing the light to be used as an emergency beacon for much longer while still remaining clearly visible.



## Developers

ProHex is easily expandable and has several features that make it a joy to work with:

* A reliable button API that makes detecting presses, holds, and long holds extremely simple
* A robust state machine based framework that makes it easy to expand in more complex ways



## Future Plans

We have great things planned for ProHex, so be sure to stay updated! Also, if you use ProHex and find it useful, or if you just want to see development continue, consider donating. We appreciate any support!

Have a suggestion? Drop us a message. While we won't be implementing every feature (we don't want to create HexBright Office!) we will consider every request and try to respond personally if we can.