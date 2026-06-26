# OSSM Lite - Lite Firmware for Open Source Sex Machine

This is a stripped down version of the OSSM firmware that I run on my personal device. It is a work in progress to add functionality I enjoy and test new features.
Others are welcome to [flash this firmware](https://fray-d.github.io/OSSM-Lite/) but no warranty or support is guaranteed beyond what you find in the documentation. Use at your own risk.
The hardware portions of the repository have been deleted in this fork and are not maintained. This firmware is built on the foundation that Research+Desire and KinkyMakers provided. Thanks to them and the community for their efforts.

Key changes:
- All modes now use Min and Max Depth... Not Depth and Stroke.
- Advanced Penetration mode has been added. Requires wired remote or a custom firmware on the RADR remote. Other controllers may be added in the future.
- Simple Penetration mode has been removed. 
- Streaming mode has been further refined. A web simple web player is in the works, but you can use the original one on the R+D documents site in the meantime.
- Stroke engine has been configured to use a linear speed instead of strokes per minute. In this way the length of the stroke no longer changes speed.
- A configuration has been added to allow you to reverse the rail.
- A configuration has been added to allow you to change how the device homes, and a second configuration to set the length of the rail. Rail length with be 180mm by default.

Roadmap:
- Add a replacement for the documentation site explaining the features of the firmware in detail. Then delete the reference to the exist incompatible docs.
- Add a configuration tool for all user configurations.
- Add a web player for funscripts.
- Continue removing dead code from the source.


