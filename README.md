# Kinect Game Input

Kinect Game Input is a Windows application that turns the Microsoft Kinect v2 sensor into a real-time body-tracking and configurable input device. It provides multi-person skeleton tracking, configurable tracking points, RGB/depth/infrared camera previews, and customizable keyboard and mouse input mappings through a Dear ImGui interface.

## Features

The application supports real-time Kinect v2 body tracking using the Kinect Body Basics D2D pipeline. The Kinect can track up to six people simultaneously, with each tracked person represented by a 25-joint skeleton. The application provides automatic person assignment as well as manual selection of Person 1 through Person 6.

Each person can have their own tracking-point configuration. Individual people can be enabled or disabled independently, allowing the user to prevent unwanted tracked people from affecting the application. Tracking-point selections are persistent and can be saved so that the configuration does not need to be recreated every time the application starts.

The application includes a configurable input-mapping system that allows tracked body movements to be associated with keyboard and mouse inputs. A single tracked point can have multiple input mappings, and multiple tracked points can use the same input. For example, the user can configure the left leg moving forward to produce W, the right leg moving forward to also produce W, and the left or right foot moving backward to produce S.

Input mappings support both single-input and held-input behavior. This allows a movement to either trigger an input once or keep an input active while the relevant movement condition remains active. The input configuration is persisted between launches.

## Camera Preview

The application provides a Camera Preview dropdown containing RGB, Depth, and Infrared modes.

RGB preview uses the Kinect v2 color stream and is based on the Color Basics D2D reference implementation located in the project's documentation.

Depth preview uses the Kinect v2 depth stream and is based on the Depth Basics D2D reference implementation located in the project's documentation.

Infrared preview uses the Kinect v2 infrared stream and is based on the Infrared Basics D2D reference implementation located in the project's documentation.

Body tracking is handled separately using the Body Basics D2D reference implementation. The Depth and Infrared preview modes are camera visualization modes and do not replace or modify the body-tracking pipeline.

The four Kinect D2D reference implementations are available in the project's docs directory:

`docs/Body Basics D2D`

`docs/Color Basics D2D`

`docs/Depth Basics D2D`

`docs/Infrared Basics D2D`

These examples are retained as development and reference material and are not presented as original application code.

## User Interface

The application uses Dear ImGui for its user interface. The interface provides controls for tracking mode, Person 1 through Person 6, individual tracking points, person enable/disable controls, input mappings, hold/single input behavior, and camera preview selection.

The application also includes a custom Windows application icon.

## Requirements

The application requires a Microsoft Kinect v2 sensor, its Kinect v2 power supply, a compatible USB 3.0 connection, and an x64 Windows PC.

Kinect v2 also requires compatible Windows graphics hardware capable of supporting the graphics functionality used by the Kinect platform. The application uses Direct2D and Windows graphics APIs for rendering and camera previews. Users do not need to install a separate legacy "DirectX 11" package simply because the application uses DirectX 11-capable graphics functionality.

## Kinect for Windows Runtime v2.2

Kinect Game Input requires the Microsoft Kinect for Windows Runtime v2.2. The Runtime provides the Kinect v2 driver and runtime environment required by applications using the sensor.

The official Microsoft Kinect for Windows Runtime v2.2 can be downloaded from:

`https://www.microsoft.com/en-us/download/details.aspx?id=100067`

The Kinect Runtime is an end-user dependency. The full Kinect for Windows SDK is a development package and is not required simply to run the released application.

## Microsoft Visual C++ Runtime

The application is built using Microsoft's MSVC C++ toolchain. The released application is distributed without application-specific DLLs or resource files that need to be manually copied beside the executable.

The Kinect Runtime remains a required system dependency. If Windows reports that an MSVC runtime component is missing, install the appropriate Microsoft Visual C++ Redistributable for the x64 application.

## Installation

Install Kinect for Windows Runtime v2.2, connect the Kinect v2 power supply, and connect the Kinect to a compatible USB 3.0 port. Allow Windows to recognize the Kinect device.

After extracting the Kinect Game Input release package, launch `KinectTool.exe`.

The released application does not require additional application-owned images, DLLs, or other resource files beside the executable. The Kinect Runtime is still required because it provides the system-level Kinect driver and runtime environment.

## Tracking

The tracking system supports automatic and manual person selection.

Auto mode automatically assigns detected people to available person slots. The application maintains the selected tracking configuration while automatically determining which detected person occupies the relevant slot.

Manual mode allows the user to select Person 1 through Person 6 directly. Each person can have an independent selection of tracked body points and can be enabled or disabled independently.

This allows configurations such as:

`Person 1 → Left Foot`

`Person 2 → Right Wrist`

`Person 3 → Disabled`

while leaving the other person slots available.

The tracking system is designed to work with multiple people simultaneously without requiring the user to manually reconfigure the application whenever the sensor detects multiple bodies.

## Input Mapping

The input-mapping system connects tracked body movements to keyboard or mouse actions.

A tracked point can have multiple mappings. Multiple tracked points can also produce the same input.

For example:

`Left leg forward → W`

`Right leg forward → W`

`Left foot backward → S`

`Right foot backward → S`

Mappings can be configured as either single inputs or held inputs.

Single input mappings trigger the configured input as an individual action, while held mappings maintain the input while the corresponding movement condition remains active.

The configured mappings are saved persistently so that the user's controls remain available after restarting the application.

## Configuration

The application stores its configuration in `config.json` alongside the executable.

The configuration stores the user's tracking-point selections, person configuration, input mappings, and other persistent application settings.

Dear ImGui interface state is stored separately in `imgui.ini`. This file stores the saved UI layout and window state.

## Resetting Configuration

To reset the application's tracking and input configuration, close Kinect Game Input and delete `config.json` from the directory containing `KinectTool.exe`.

Launch the application again and the application will recreate its default configuration.

If the user also wants to reset the saved Dear ImGui interface layout, `imgui.ini` can be deleted as well.

Deleting `imgui.ini` resets the saved UI layout without being intended to remove the application's tracking and input configuration.

## Repository Structure

The repository is organized to separate application source code, resources, documentation, configuration, generated build output, and distributable releases.

The repository contains source and header files for the application, an `assets` directory for application resources such as the application icon, a `docs` directory containing Kinect reference implementations and documentation, a build directory for generated development output, and a distribution directory for release-ready application files.

The repository does not use the previous `assets/archive` concept. Application assets and release/distribution files are kept separate so that the source repository remains suitable for public GitHub development and transparency.

## Application Icon

The application uses a custom Kinect Game Input Windows icon stored under `assets/icons`.

The icon is integrated into the Windows executable and is used for the application window, taskbar, and executable representation.

## Building From Source

Kinect Game Input is developed for Windows using Microsoft's C++ toolchain. The development environment uses MSVC / Visual C++ Build Tools, the Windows SDK, Kinect for Windows SDK v2 development files, and Windows graphics APIs including Direct2D.

The full Visual Studio IDE is not required if the project is built using the installed Visual C++ Build Tools and the project's configured build process.

The Kinect SDK is a development dependency and reference source. End users running the compiled release should install the Kinect Runtime rather than the full SDK.

## Kinect SDK Reference Material

The project contains Kinect v2 Direct2D reference implementations for Body, Color, Depth, and Infrared functionality.

The Body Basics D2D example provides the reference for the Kinect body-tracking pipeline.

The Color Basics D2D example provides the reference for the RGB camera preview.

The Depth Basics D2D example provides the reference for the depth camera preview.

The Infrared Basics D2D example provides the reference for the infrared camera preview.

These examples are maintained in the `docs` directory for development, transparency, and reference purposes. Microsoft-provided source code and documentation remain subject to their applicable Microsoft licenses and terms.

## Architecture

The application is designed around a reusable Kinect tracking layer.

The general tracking data flow is:

`Kinect v2 → Kinect acquisition → Body tracking → Skeleton/joint data → Tracking interpretation → Input mapping`

Camera previews use the Kinect camera streams independently:

`Kinect v2 → Color stream → RGB Preview`

`Kinect v2 → Depth stream → Depth Preview`

`Kinect v2 → Infrared stream → Infrared Preview`

This separation allows the body-tracking system to remain independent from the camera preview system and makes the underlying tracking functionality potentially reusable by future applications and game integrations.

## Release Package

The source repository contains the complete development project, including application source code, documentation, Kinect reference material, application resources, and build configuration.

The compiled release is distributed separately as a ZIP package containing the finished `KinectTool.exe`.

The released executable is intended to be self-contained with respect to application-owned resources: no additional application-specific DLLs, images, or resource files need to be manually placed beside the executable.

The Microsoft Kinect Runtime v2.2 remains an external system dependency and must be installed on the user's computer.

## Troubleshooting

If the Kinect is not detected, verify that the Kinect v2 power supply is connected, the sensor is connected through a compatible USB 3.0 connection, Kinect for Windows Runtime v2.2 is installed, and Windows recognizes the Kinect device.

If the application does not launch, verify that the correct x64 release is being used, the Kinect Runtime v2.2 is installed, and the Kinect is connected correctly. If Windows reports a missing MSVC runtime component, install the appropriate Microsoft Visual C++ Redistributable.

If body tracking does not work, verify that the Kinect is connected and recognized by Windows, that a person is visible to the sensor, and that the sensor has a clear view of the player.

If the Depth preview does not work, select `Camera Preview → Depth` and verify that the Kinect is connected and functioning correctly.

If the Infrared preview does not work, select `Camera Preview → Infrared` and verify that the Kinect is connected and functioning correctly.

## Credits

Kinect Game Input is an independent application built around Microsoft's Kinect v2 technology, Kinect for Windows SDK/runtime, and Windows APIs.

Microsoft Kinect for Windows SDK v2 reference material is used for development and reference.

Microsoft-provided reference material remains subject to its applicable licenses and terms.

## License

See `LICENSE` for the license governing the Kinect Game Input application.

Third-party and Microsoft reference material remains subject to its respective licenses and terms.