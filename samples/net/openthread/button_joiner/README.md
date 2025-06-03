# OpenThread Button Joiner and Commissioner Sample

## Overview

This sample demonstrates the OpenThread commissioning and joining process. It can be configured to build an application that acts either as an OpenThread **Commissioner** or an OpenThread **Joiner**.

The "button_joiner" name implies that a button press is typically used to initiate the joining process on the Joiner device. The Commissioner device starts the commissioning process, allowing new devices (Joiners) to join the Thread network.

This sample showcases:
-   Initializing the OpenThread stack.
-   Configuring a device as an OpenThread Commissioner.
-   Configuring a device as an OpenThread Joiner.
-   Using a button press (or other user input, depending on the board's capabilities and application logic) to trigger the joining process.
-   Basic Thread network formation and joining.

Typically, two devices are required to test this sample: one flashed with the Commissioner role and another with the Joiner role.
Alternatively, the Joiner device can be tested using an OpenThread Border Router (OTBR) and an external commissioner tool (e.g., a smartphone app or a computer application that can perform Thread commissioning).

## Requirements

-   At least two Zephyr-supported boards with 802.15.4 radios compatible with OpenThread (e.g., `cc1352p7_lp` as specified in `sample.yaml`).
-   A way to trigger an action on the Joiner device (e.g., a button).
-   Zephyr development environment set up.

## Building and Running

This sample can be built for two distinct roles: Commissioner and Joiner. You will need to build and flash each role onto a separate device. The Kconfig fragments `commissioner.conf` and `joiner.conf` are used to enable the respective roles.

The `sample.yaml` file defines build configurations for these roles, specifically targeting the `cc1352p7_lp` board. You can adapt the board name for your hardware.

### Building as a Commissioner

To build the sample as an OpenThread Commissioner:

```shell
west build -b <your_board_name> samples/net/openthread/button_joiner -- -DEXTRA_CONF_FILE=commissioner.conf
```

For example, using `cc1352p7_lp`:

```shell
west build -b cc1352p7_lp samples/net/openthread/button_joiner -- -DEXTRA_CONF_FILE=commissioner.conf
```

Then, flash the built image to your first device.

### Building as a Joiner

To build the sample as an OpenThread Joiner:

```shell
west build -b <your_board_name> samples/net/openthread/button_joiner -- -DEXTRA_CONF_FILE=joiner.conf
```

For example, using `cc1352p7_lp`:

```shell
west build -b cc1352p7_lp samples/net/openthread/button_joiner -- -DEXTRA_CONF_FILE=joiner.conf
```

Then, flash the built image to your second device.

## Expected Behavior

1.  **Commissioner Device:**
    *   After flashing and resetting, the Commissioner device will initialize the OpenThread stack.
    *   Upon a trigger (e.g., pressing a designated button), it will start the commissioning process. This typically involves forming a new Thread network (if one doesn't already exist with its credentials) and opening the network to allow new joiners.
    *   The device might indicate its status (e.g., ready to commission, commissioning active) via LEDs or console logs.

2.  **Joiner Device:**
    *   After flashing and resetting, the Joiner device will initialize the OpenThread stack.
    *   If the device has not been previously commissioned to a Thread network, it will automatically start the joining process. It will continuously scan for an available Thread network being commissioned and attempt to join, retrying until successful.
    *   Pressing the designated button on the Joiner device will trigger a factory reset of its OpenThread network settings (`ot factoryreset`). After the reset, it will (re)start the joining process, scanning and attempting to join a network.
    *   The device will indicate its current operational state (e.g., initializing, scanning, attempting to join, joined, factory reset) via LEDs or console logs.

3.  **Interaction:**
    *   Start the Commissioner device first.
    *   Once the Commissioner is ready (check console logs or LED status if available), trigger the joining process on the Joiner device (e.g., press the button).
    *   Observe the console logs and/or LED status on both devices. The Joiner should indicate it has successfully joined the network (e.g., via a specific LED pattern or log message), and the Commissioner might indicate that a new device has joined.

## Console Output and Debugging
Enable console output (e.g., via UART) to observe the OpenThread logs and the sample's status messages. This is crucial for understanding the state of each device and troubleshooting any issues.

You can typically access OpenThread CLI commands if `CONFIG_OPENTHREAD_CLI` is enabled in the project configuration (it might be enabled by default or in the board's defconfig). This allows you to inspect the Thread network parameters, device state, etc.

Example CLI commands:
```shell
# On either device, after joining/commissioning
ot state
ot ipaddr
ot panid
ot channel
```

## Testing with an External Commissioner

Instead of using the Commissioner role provided by this sample, you can test the Joiner device with an existing OpenThread Border Router (OTBR) and an external commissioning client. This is a common setup for adding devices to a Thread network.

1.  **Prerequisites:**
    *   Build and flash the `button_joiner` sample onto your device using the `joiner.conf` configuration (as described in "Building as a Joiner").
    *   Have an operational OpenThread Border Router (OTBR) that has formed a Thread network.
    *   An external commissioning client. This could be:
        *   A smartphone app (e.g., the nRF Thread Topology Monitor, or other Thread-certified commissioner apps).
        *   The web GUI provided by some OTBRs.
        *   The `ot-ctl` command-line tool on the OTBR or another connected system.
        *   A dedicated PC-based commissioning tool.

2.  **Commissioning Steps (General):**
    *   Ensure the Joiner device is powered on. As described in its "Expected Behavior", it will automatically attempt to join if uncommissioned, or you can trigger a factory reset and joining attempt via its button.
    *   On your external commissioning client, initiate the process to add a new device. This often involves scanning for joinable devices or entering the Joiner's EUI64 (if known, often printed in device logs) or a Joiner Credential (e.g., `J01NME` is a common default for testing, but the actual credential depends on the Joiner's setup). Some commissioners might use a QR code if the Joiner supports it.
    *   Follow the instructions provided by your commissioning client. It will guide the Joiner through the secure commissioning process.
    *   Observe the Joiner's console logs and/or LED status for indications of the joining progress and success. The commissioning client should also report the status.
