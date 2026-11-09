# CTRL Mixbus
### Native Stereo Mixing and Processing Plugin for Akai MPC

**Project:** CTRL Mixbus  
**Plugin Type:** VST Audio Effect  
**Developer:** Tony Alonso Tech
**Primary Platform:** Akai MPC Standalone  
**Distribution:** [Open MPC Plugin Catalog](https://open-mpc.github.io/)  
**Status:** In Development

---

## 1. Overview

CTRL Mixbus is a professional stereo mixing and processing plugin designed for Akai MPC standalone hardware.

Inspired by the workflow and functionality of professional DJ mixers, CTRL Mixbus combines essential mixing tools, a DJM style three-band isolator, adjustable filtering, stereo manipulation, and dynamics processing into a single VST effect.

The plugin is being developed with a particular emphasis on:

- Native operation within MPC standalone firmware.
- Low CPU and memory consumption.
- Full touchscreen operation.
- Hardware Q-Link integration.
- Native MPC preset management.
- Portable presets compatible across Windows, macOS, Linux, and MPC.
- Consistent DSP behavior across supported architectures.

CTRL Mixbus is intended to function as a native MPC audio effect without requiring external applications, background services, or additional runtime installations.

## 2. Supported Hardware and Firmware

CTRL Mixbus will provide separate builds for the two MPC hardware generations.

| Hardware generation | Architecture | Target firmware |
|---|---|---|
| MPC Gen 1 | ARM32 / ARMHF | MPC OS 2.x / 3.x |
| MPC Gen 2 | ARM64 / AArch64 | MPC OS 3.x |

### Gen 1 Devices

Target hardware includes:

- MPC Live
- MPC Live II
- MPC One
- MPC One+
- MPC X
- MPC Key 61
- Akai Force

### Gen 2 Devices

Target hardware includes:

- MPC Live III
- MPC One G2
- MPC Key 37 G2
- Akai Force G2

**Important:** Hardware and firmware compatibility will be individually tested and documented. A successful build for an architecture does not automatically constitute hardware compatibility.

### Firmware Compatibility

The development process will prioritize compatibility with the Open MPC plugin toolchain and its established release requirements.

Gen 1 builds will use a compatible ARMHF toolchain and glibc baseline. Gen 2 builds will use a separate AArch64 toolchain.

## 3. Signal Processing Architecture

CTRL Mixbus uses a fixed processing chain:

1. Input Gain
2. Phase Inversion
3. Stereo Width
4. Bass Mono
5. Balance
6. Three-Band Isolator EQ
7. Low-Pass Filter
8. High-Pass Filter
9. Noise Gate
10. Output Level

Every processing stage includes an independent ON/OFF bypass control.

Bypass controls will be operated through the MPC touchscreen.

### 3.1 Input Gain

- Adjustable gain: -∞ dB to +24 dB.
- Independent bypass.
- Stereo input metering.

### 3.2 Phase Inversion

- Simultaneous phase inversion of both stereo channels.
- Single ON/OFF control.
- Independent bypass.

### 3.3 Stereo Width

- Adjustable stereo width.
- Normal stereo operation through to mono.
- Mid/Side-based processing.
- Independent bypass.

### 3.4 Bass Mono

- Adjustable crossover frequency.
- Low-frequency stereo information converted to mono.
- Stereo information above the crossover retained.
- Independent bypass.

### 3.5 Balance

- Adjustable left/right balance.
- Center position maintains equal channel levels.
- Independent bypass.

### 3.6 Three-Band Isolator EQ

Based on the isolator behavior of the Pioneer DJM-900NXS2.

Three independent frequency bands:

- LOW
- MID
- HIGH

Each band will have independent level adjustment and full-kill capability.

The entire EQ section will have an independent bypass control.

### 3.7 Low-Pass Filter

Adjustable low-pass filter with:

- Cutoff frequency.
- Resonance.
- Selectable filter curve:
  - 12 dB/octave
  - 24 dB/octave
  - 36 dB/octave
  - 48 dB/octave
  - 96 dB/octave
  - Brickwall

Independent bypass.

### 3.8 High-Pass Filter

Adjustable high-pass filter with:

- Cutoff frequency.
- Resonance.
- Selectable filter curve:
  - 12 dB/octave
  - 24 dB/octave
  - 36 dB/octave
  - 48 dB/octave
  - 96 dB/octave
  - Brickwall

Independent bypass.

### 3.9 Noise Gate

Configurable stereo noise gate with:

- Threshold.
- Attack.
- Hold.
- Release.

Stereo-linked detection will be used to maintain stereo image stability.

Independent bypass.

### 3.10 Output Level

- Adjustable output gain: -∞ dB to +12 dB.
- Independent bypass.
- Stereo output metering.

## 4. MPC User Interface

CTRL Mixbus will include a dedicated native MPC touchscreen interface.

The interface will be designed around the MPC hardware display and Q-Link layout.

### Q-Link Integration

A maximum of 16 rotary controls will be exposed on any individual interface page.

This ensures that the available Q-Links can correspond directly with the visible controls.

The following interaction rules will apply:

- Rotary parameters are controlled using Q-Links.
- ON/OFF switches are controlled exclusively through the touchscreen.
- Bypass switches are touchscreen-operated.
- Phase inversion is touchscreen-operated.
- All parameter changes must be reflected immediately in the interface.
- Q-Link assignments must follow the currently selected interface page.

### Interface Pages

The initial interface will be divided into the following pages.

**MAIN / MIX**

- Input Gain
- Stereo Width
- Bass Mono Crossover
- Balance
- Output Level
- Input Meter
- Output Meter
- Processing bypass controls

**EQ / FILTER**

- LOW EQ
- MID EQ
- HIGH EQ
- Low-Pass Cutoff
- Low-Pass Resonance
- Low-Pass Curve
- High-Pass Cutoff
- High-Pass Resonance
- High-Pass Curve
- EQ and filter bypass controls

**GATE**

- Threshold
- Attack
- Hold
- Release
- Gate bypass
- Gain reduction visualization

Additional pages may be introduced if necessary, provided the 16-control maximum is maintained.

### Visual Feedback

The MPC interface must provide meaningful visual feedback.

Required visual elements include:

- Stereo input level meter.
- Stereo output level meter.
- Live filter response visualization.
- Visual indication of filter cutoff and resonance.
- Gate activity / gain reduction indication.
- Clearly visible ON/OFF states for every processing stage.
- Current parameter values.

Visual elements must update without interfering with audio processing.

## 5. Preset Management

CTRL Mixbus will include its own portable preset system.

The preset format must remain compatible across all supported platforms.

A preset created on Windows must be loadable on macOS, Linux, and MPC without conversion.

### Factory Presets

The initial release will contain exactly one factory preset:

**Init**

This preset will initialize every parameter to its defined default value.

### User Presets

Users must be able to:

- Save their current settings as a new preset.
- Load previously saved presets.
- Rename user presets.
- Delete user presets.
- Restore the Init preset.

### Cross-Platform Compatibility

Presets will use a documented, versioned, platform-independent file format.

The format will not depend on operating-system-specific paths or binary serialization.

Preset compatibility must be maintained between:

- Windows desktop.
- macOS desktop.
- Linux desktop.
- MPC Gen 1.
- MPC Gen 2.

MPC host preset integration will also be implemented so CTRL Mixbus can participate in the MPC preset workflow.

## 6. Dependencies and Packaging

All required runtime dependencies must be included within the distributed plugin package wherever licensing and platform requirements permit.

The plugin must not require users to independently install:

- Additional DSP libraries.
- External runtime libraries.
- Background services.
- Separate processing applications.
- Additional audio-processing frameworks.

The final plugin package must include all required plugin assets, interface resources, and supporting files.

Dependencies that cannot legally or technically be bundled must be explicitly documented.

### MPC Package Structure

The MPC release will follow the Open MPC plugin packaging conventions.

The final release package is expected to contain:

- Native VST2 shared library.
- MPC plugin metadata.
- Native touchscreen skin.
- Q-Link mapping configuration.
- Required plugin assets.
- Portable preset support.
- Installation and removal scripts.
- Installation documentation.

The exact directory structure will follow the Open MPC release requirements in effect at the time of publication.

## 7. Installation and Distribution

CTRL Mixbus will be distributed through the official Open MPC ecosystem.

**Official distribution location:**

https://open-mpc.github.io/

Once published, users will be able to find CTRL Mixbus in the Open MPC plugin catalog.

### Open MPC Plugin Manager

The preferred installation method will be the Open MPC Plugin Manager.

Users will be able to:

1. Open the Plugin Manager on their MPC.
2. Browse the available plugin catalog.
3. Locate CTRL Mixbus.
4. Select the appropriate available release.
5. Install CTRL Mixbus directly onto their MPC.
6. Load CTRL Mixbus from the MPC plugin browser.

Updates and removal will follow the Plugin Manager's supported workflows.

### Desktop Installer

CTRL Mixbus will also be distributed through the Open MPC desktop installer where supported.

The installer supports Windows, macOS, and Linux computers and can install compatible plugin packages onto connected MPC hardware.

### Firmware-Specific Installation

Installation procedures will be documented for each supported firmware generation.

MPC OS 2.x devices require a separate installation route because the Open MPC Plugin Manager is designed for MPC OS 3.x.

No manual firmware modification should be necessary for a standard CTRL Mixbus installation.

## 8. Performance Requirements

CTRL Mixbus is intended to run alongside other MPC instruments, effects, and audio tracks.

CPU and memory efficiency are therefore essential.

Development requirements include:

- No dynamic memory allocation in the real-time audio processing path.
- No blocking filesystem operations during audio processing.
- No unnecessary background processing.
- Efficient stereo DSP implementations.
- Parameter smoothing to minimize audible artifacts.
- Stable operation at the MPC host's supported sample rate and buffer size.
- Efficient filter and gate processing.
- Consistent behavior across ARM32 and AArch64 builds.

Performance testing will be conducted on actual MPC hardware.

Each hardware generation will receive its own performance validation.

## 9. Testing and Quality Assurance

Before a release is considered production-ready, the following areas must be tested.

### DSP Testing

- Frequency response.
- Filter slope accuracy.
- Filter resonance.
- Isolator crossover behavior.
- Phase integrity.
- Stereo width accuracy.
- Bass mono operation.
- Gate attack, hold, and release.
- Input and output gain accuracy.
- Clipping and headroom behavior.
- Bypass transitions.

### MPC Integration Testing

- Plugin loading.
- Audio input and output.
- Touchscreen interface.
- Q-Link assignments.
- Touchscreen bypass controls.
- Parameter automation.
- Preset loading and saving.
- Project recall.
- Project reopening.
- Interface page switching.
- Meter and visualization performance.

### Hardware Testing

Separate validation will be required for:

- Gen 1 ARM32 hardware.
- Gen 2 AArch64 hardware.
- Supported MPC OS versions.
- Supported Hakai firmware versions.

A release must not be marked as hardware-verified until it has been tested on the corresponding physical hardware.

## 10. Release Documentation

Every published release must include or reference:

- Complete source code.
- Required headers and submodules.
- Build scripts and build configuration.
- Architecture-specific compiler flags.
- Compiler and toolchain versions.
- Plugin metadata.
- MPC touchscreen assets.
- Q-Link configuration.
- Preset format documentation.
- Installation instructions.
- Release screenshots.
- Exact source commit or Git tag.
- SHA-256 checksums.
- Hardware compatibility and testing results.

Releases will use versioned source tags to ensure that published binaries can be traced back to their exact source.

## 11. Development Roadmap

| Milestone | Description |
|---|---|
| 0.1 | Initial DSP engine and parameter architecture |
| 0.2 | Complete processing chain |
| 0.3 | Portable preset engine |
| 0.4 | Native MPC interface and Q-Link integration |
| 0.5 | Gen 1 ARM32 build and hardware testing |
| 0.6 | Gen 2 AArch64 build and hardware testing |
| 0.7 | Hakai firmware compatibility validation |
| 0.8 | Performance optimization and release testing |
| 1.0 | Initial production release |

Milestones may be adjusted as development and hardware testing progress.

## 12. Project References

The following projects and resources guide development.

**Open MPC Plugin Catalog**  
https://open-mpc.github.io/

**Open MPC VST Plugin Development Repository**  
https://github.com/open-mpc/mpc-vst-plugins

**Airwindows LinuxVST Source Code**  
https://github.com/airwindows/airwindows/tree/master/plugins/LinuxVST/src

These projects provide reference implementations, build tooling, plugin packaging conventions, and examples of native MPC plugin development.

---

## 13. Project Status

**CTRL Mixbus is currently in development.**

The initial development phase will focus on the Windows desktop implementation, allowing the DSP engine and preset system to be fully developed and tested before moving to MPC hardware.

The MPC implementation will subsequently use the same underlying DSP architecture, with platform-specific VST2 integration, touchscreen interfaces, and hardware optimizations.

The ultimate goal is to provide a complete, self-contained, professional stereo processing solution that installs directly through the Open MPC ecosystem and operates natively on supported Akai MPC standalone hardware.

**CTRL Projects**  
CTRL Mixbus
