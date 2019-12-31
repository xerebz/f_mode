# f_mode

This application forwards packets between pairs of interfaces.

## Building

Use CMake to generate and ninja to build.

## Running

1. Allocate and mount hugepages.
2. Bind userspace drivers to the forwarding interfaces.
3. Run as root.
