## AutoHold research on Ford Fusion 2017

### MS CAN (FCIM and other non-important modules)

Toggling on and off is done by sending PRESSED or NOT PRESSED states to the GWM, which
then communicates with the ABS through GWM by sending the signal from below. This is FCIM's signal: 

```
CAN ID: 1050
Byte index: 2 (pressed 0x08, not pressed 0x00)
Bit index: 20-19 (pressed 1, not pressed 0)
```

To receive current AutoHold state, read this message:

```
CAN ID: 765
Byte index: 5
Bit index: 47 (enabled 1, disabled 0)
```

### HS2 CAN (ABS and other important modules)

Toggling on and off is done by sending two short signals: one for PRESSED and one for NOT PRESSED.
Those have to have a delay in between them of at least 5ms (?) but not longer than 1000ms (?).
10ms works all of the time, so it is recommended.

```
CAN ID: 1073
Byte index: 0
Bit index: 0 (pressed 1, not pressed 0)
```

To receive current AutoHold state, read this message:

```
CAN ID: 1054
Byte index: 0
Bit index: 0 (enabled 1, disabled 0)
```

### HS3 CAN

Toggling on and off is either not possible on a 2017 GWM or unknown how to implement.

To receive current AutoHold state, read this message:

```
CAN ID: 1054
Byte index: 0
Bit index: 0 (enabled 1, disabled 0)
```

