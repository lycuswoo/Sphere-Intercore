
# Sample: Inter-core communication

This sample demonstrates how to exchange messages between applications running on the high-level and real-time capable cores, and are based on [Intercore Communication example by Microsoft](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/IntercoreComms).

Once per second the high-level application sends a color of choice (Red, Blue, Green) "Red-%d", where %d is an incrementing counter to the real-time capable application - color of choice will change every 10 increment. The real-time capable application prints the received data, converts any upper-case characters to lower-case and vice versa, and echoes the message back with a customized message to the high-level application.

The high-level application uses the following Azure Sphere libraries and includes [beta APIs](https://docs.microsoft.com/azure-sphere/app-development/use-beta):

|Library   |Purpose  |
|---------|---------|
|application.h |Communicates with and controls real-time capable applications |
|log.h |Displays messages in the Visual Studio Device Output window during debugging |
|eventloop.h |Invoke handlers for timer events |

The real-time capable features used in the sample are in Beta.

To use this sample, clone the repository locally if you haven't already done so:

```
git clone https://github.com/lycuswoo/Sphere-Intercore.git
```

## Prerequisites

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design.
1. (Optional for M4 debug msg) A USB-to-serial adapter (for example, [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311)) to connect the real-time capable core UART to a USB port on your PC.
1. (Optional for M4 debug msg) A terminal emulator (such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/.)) to display the output.

## Set up hardware to display output

To prepare your hardware to display output from the sample, see RTApp readme for setup

## Build and run the apps

The applications in this sample run as partners. Make sure that they're designated as partners as described in [sideload more than one application](https://docs.microsoft.com/azure-sphere/app-development/sideload-app#sideload-more-than-one-application) so that sideloading one doesn't delete the other.

Note - RT App need to run first so the Highlevel App is able to open a intercore communication socket

### Build and run the RTApp

See the following Azure Sphere Quickstarts to learn how to build, and deploy a real-time application:

   -  [with Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-real-time-application)
   -  [with VS Code on Windows or Linux](https://docs.microsoft.com/azure-sphere/install/qs-real-time-app-vscode)
   -  [on the Windows or Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-real-time-app-cli)

### Build and run the high level application

See the following Azure Sphere Quickstarts to learn how to build and deploy a high-level application:

   -  [with Visual Studio](https://docs.microsoft.com/azure-sphere/install/qs-blink-application)
   -  [with VS Code](https://docs.microsoft.com/azure-sphere/install/qs-blink-vscode)
   -  [on the Windows command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-cli)
   -  [on the Linux command line](https://docs.microsoft.com/azure-sphere/install/qs-blink-linux-cli)

## Observe the output

The high-level application output will be displayed in the Output window in Visual Studio.

```sh
Remote debugging from host 192.168.35.1
High-level intercore application.
Sends data to, and receives data from the real-time core.
Sending: Red-0
Received 13 bytes: rED-0
Sending: Red-1
Received 13 bytes: rED-1
Sending: Red-2
```

The real-time core application output will be sent to the serial terminal for display.

```sh
IntercoreComms_RTApp_MT3620_BareMetal
App built on: Dec 12 2019, 10:17:38
Received message of 33 bytes:
  Payload (13 bytes as text): Red-0
Received message of 33 bytes:
  Payload (13 bytes as text): Red-1
Received message of 33 bytes:
  Payload (13 bytes as text): Red-2
```