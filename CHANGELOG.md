# CIRQUE TOUCH FIRMWARE UPDATER FOR CHROME OS (cirque_touch_fw_update)
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.1] - 2025-04-10

### Added

- Implemented support for using firmware binary files as input.

### Changed

- Modified some failure output strings to avoid duplicate messages.
- Added a device path to the output message upon failure to get the device firmware version.

## [2.0.7] - 2024-10-11

### Added

- Added a startup option to list devices.
- Added device filepath to the output message.
- Added a startup option to display tool version information.
- Added firmware revision to the device attribute output.
- Added support for handling errors and for communicating with the device in bootloader mode.
- Added support for big-endian firmware.

### Fixed

- Fixed an error in Fletcher-32 checksum calculation.

### Changed

- Improved HEX file address parsing.
- Increased delays between calls to the firmware to avoid failure in getting device status.
- Changed the detection of the firmware version to return an error code.

### Removed

- Removed all output except for the firmware version number when the tool is used to get the firmware version.

## [1.0.5] - 2019-07-19

### Added

- Enabled access to the device to allow the code to work without running as root.

## [1.0.4] - 2019-07-18

### Removed

- Removed exception handling to enable the code to be built in Chrome OS SDK.

## [1.0.3] - 2019-07-18

### Changed

- Updated Makefile to use build environment.

## [1.0.2] - 2019-07-16

### Changed

- Changed the Cirque software license to Apache.
- Updated device status evaluation to support new bootloader code.

## [1.0.1] - 2019-05-31

### Fixed

- Included <stdexcept> to fix an ebuild error.

## [1.0.0] - 2019-05-29

### Added

- Initial release.
