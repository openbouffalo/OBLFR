# Component: Button
Taken from the esp-iot-solution and modified for Bouffalolab SDK.

After creating a new button object by calling function `button_create()`, the button object can create press events, every press event can have its own callback.

List of supported events:
 * Button pressed
 * Button released
 * Button pressed - repeated
 * Button single click
 * Button double click
 * Button long press start
 * Button long press hold
 * Button long press done

There are two ways this driver can handle buttons:
1. Buttons connected to standard digital GPIO
2. Multiple buttons connected to single ADC channel
