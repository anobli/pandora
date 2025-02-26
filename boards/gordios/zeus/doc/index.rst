.. zephyr:board:: zeus

Overview
********

The Gordios Zeus board is a smart power control and measurement device, built
around the Texas Instruments SimpleLink |trade| multi-standard CC1352P7 wireless MCU.
It functions as a connected power relay, capable of measuring power consumption.
The Zeus board is designed for integration into outlets to switch power on or off,
control electric heaters, or act as a smart light switch.

See the `TI CC1352P7 Product Page`_ for details on the SoC.


Hardware
********

The Zeus board leverages the CC1352P7 wireless MCU as its core component.
Hardware features are tailored for its application as a smart power controller,
including circuitry for power relay operation and energy monitoring.
For debugging and programming, it provides access to the necessary SoC interfaces,
compatible with tools like TI's XDS110.

The CC1352P7 wireless MCU has a 48 MHz Arm |reg| Cortex |reg|-M4F SoC and an
integrated sub-1GHz and 2.4 GHz transceiver with integrated 20dBm power amplifier
(PA) supporting multiple protocols including Bluetooth |reg| Low Energy and IEEE
|reg| 802.15.4.

See the `TI CC1352P7 Product Page`_ for additional details on the MCU.

Connections and IOs
===================

The I/O signals of the CC1352P7 on the Zeus board are utilized for its
specific functions, including:

*  Control signals for the power relay.
*  Interface for the power measurement circuitry (e.g., ADC inputs, SPI/I2C for an energy monitoring IC).
*  Communication interfaces (e.g., UART for debugging or external communication).
*  GPIOs for status indicators (LEDs) or user inputs (buttons), if present.

Please refer to the Zeus board schematics for detailed pin assignments and functions.

Programming and Debugging
*************************

Before flashing or debugging, ensure that the necessary JTAG/SWD signals (TMS, TCK,
TDO, TDI, nRESET) are correctly connected to your XDS110 debugger.
For serial console output via the XDS110, ensure the UART TXD and RXD lines
are also connected.

Prerequisites:
==============

#. Ensure the XDS-110 emulation firmware on the board is updated.

   Download and install the latest `XDS-110 emulation package`_.

   Follow these `xds110 firmware update directions
   <http://software-dl.ti.com/ccs/esd/documents/xdsdebugprobes/emu_xds110.html#updating-the-xds110-firmware>`_

   Note that the emulation package install may place the xdsdfu utility
   in ``<install_dir>/ccs_base/common/uscif/xds110/``.

#. Install OpenOCD

   Currently, OpenOCD doesn't support the CC1352P7.
   Until its support get merged, we have to builld a downstream version that could found `here <https://github.com/anobli/openocd>`_.
   Please refer to OpenOCD documentation to build and install OpenOCD.

   For your convenience, we provide a `prebuilt binary <https://github.com/anobli/openocd/actions/runs/10566225265>`_.

.. code-block:: console

   $ unzip openocd-810cb5b21-x86_64-linux-gnu.zip
   $ chmod +x openocd-x86_64-linux-gnu/bin/openocd
   $ export OPENOCD_DIST=$PWD/openocd-x86_64-linux-gnu

By default, zephyr will try to use the OpenOCD binary from SDK.
You will have to define :code:`OPENOCD` and :code:`OPENOCD_DEFAULT_PATH` to use the custom OpenOCD binary.

Flashing
========

Applications for the ``zeus`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ screen <tty_device> 115200

Replace :code:`<tty_device>` with the port where the XDS110 application
serial device can be found. For example, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: zeus
   :goals: build flash
   :gen-args: -DOPENOCD=$OPENOCD_DIST/bin/openocd -DOPENOCD_DEFAULT_PATH=$OPENOCD_DIST/share/openocd

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: zeus
   :maybe-skip-config:
   :goals: debug
   :gen-args: -DOPENOCD=$OPENOCD_DIST/bin/openocd -DOPENOCD_DEFAULT_PATH=$OPENOCD_DIST/share/openocd

Bootloader
==========

The ROM bootloader on CC13x2x7 and CC26x2x7 devices is enabled by default. The
bootloader will start if there is no valid application image in flash or the
so-called backdoor is enabled (via Kconfig option
:kconfig:option:`CONFIG_CC13X2_CC26X2_BOOTLOADER_BACKDOOR_ENABLE`).
If a physical button is used to trigger the backdoor on the Zeus board,
it must be held down during reset. See the bootloader documentation in chapter 10 of the `TI
CC13x2x7 / CC26x2x7 Technical Reference Manual`_ for additional information.

References
**********
*(This section can be expanded with links to Zeus board specific schematics or user guides if available)*

.. _TI CC1352P7 Product Page:
   https://www.ti.com/product/CC1352P7

.. _TI CC13x2x7 / CC26x2x7 Technical Reference Manual:
   https://www.ti.com/lit/ug/swcu192/swcu192.pdf

..  _XDS-110 emulation package:
   http://processors.wiki.ti.com/index.php/XDS_Emulation_Software_Package#XDS_Emulation_Software_.28emupack.29_Download
