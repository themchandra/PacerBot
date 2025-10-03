# Documentations


## UART Message Structure
The message structure will be as follows:
a header (for syncing), a command (x bits), a value (x bits), and followed by a checksum (x bits)

```
HEADER | START | XXXXXXXX | CHECKSUM
HEADER | STOP  | XXXXXXXX | CHECKSUM

HEADER | SPEED | THROTTLE | CHECKSUM
HEADER | TURN  | DEGREES  | CHECKSUM

```

Header will be constant and to be used as the start of a message. Header values will be different when receiving from/transmitting to the STM32.
