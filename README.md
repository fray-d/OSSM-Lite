# OSSM Lite - Lite Firmware for Open Source Sex Machine

This is a stripped down version of the OSSM firmware that I run on my personal device. It is a work in progress to add functionality I enjoy and test new features.
Others are welcome to [read the docs and flash this firmware](https://fray-d.github.io/OSSM-Lite/) but no warranty or support is guaranteed beyond what you find in the documentation. Use at your own risk.
The hardware portions of the repository have been deleted in this fork and are not maintained. This firmware is built on the foundation that Research+Desire and KinkyMakers provided. Thanks to them and the community for their efforts.

Key changes:
- The function button on the reference board is now active with some assigned functions.
- [LED status enabled](https://fray-d.github.io/OSSM-Lite/led.html) 
- All modes now use Min and Max Depth... Not Depth and Stroke.
    - Depth and Stroke remain in BLE to support legacy controllers
- Advanced Penetration mode has been added. 
    - Requires wired remote or a custom firmware on the RADR remote.
    - Other controllers may be added in the future.
- Simple Penetration mode has been removed. 
- Streaming mode has been further refined. 
    - An funscript player is provided in [the documentation.](https://fray-d.github.io/OSSM-Lite/funscript.html)
- Stroke engine has been configured to use a linear speed instead of strokes per minute. 
- User configurations are supported [See docs](https://fray-d.github.io/OSSM-Lite/) 
    - Set homing style (None, Default, Single-Sided, Double-Tap)
    - Set rail length for None and Sinle-Sided homing (180mm default)
    - Supports even the largest of rails...
    - Reverse rail direction
    - Rename device
    - WiFi
    - Advanced motor params (RPM, Steps per Rev, Belt Pitch, etc)

Roadmap:
- Add a tool for sharing advanced mode presets manually.
- Continue removing dead code from the source.

Feature suggestions:
- Add an issue or ping me on discord to suggest a feature or if you find a bug. 
    - Please be as detailed as you can so I can reproduce the issue. 
    - Double check you're on the latest version, please. I may have already addressed it.
