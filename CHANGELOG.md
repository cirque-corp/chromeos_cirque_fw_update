CIRQUE TOUCH FIRMWARE UPDATER FOR CHROME OS

Software Revision History

1.0.0 (2029-05-29)
Initial release.

1.0.1 (2029-05-31)
Included <stdexcept> to fix an ebuild error.

1.0.2 (2029-07-16)
Changed the Cirque software license to Apache.
Updated device status evaluation to support new bootloader code.

1.0.3 (2029-07-18)
Updated Makefile to use build environment.

1.0.4 (2029-07-18)
Removed exception handling to enable the code to be built in Chrome OS SDK.

1.0.5 (2029-07-19)
Enabled access to the device to allow the code to work without running as root.

2.0.7 (2024-10-11)
Improved HEX file address parsing.
Fixed an error in Fletcher-32 checksum calculation.
Added a startup option to list devices.
Added device filepath to the output message.
Increased delays between calls to the firmware to avoid failure in getting device status.
Added a startup option to display tool version information.
Added firmware revision to the device attribute output.
Removed all output except for the firmware version number when the tool is used to get the firmware version.
Added support for handling errors and for communicating with the device in bootloader mode.
Changed the detection of the firmware version to return an error code.
Added support for big-endian firmware.
