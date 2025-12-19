# Micronucleus bootloader

## Checkout the submodule

To checkout the submodule execute the following commands within the repository:

``` console
git submodule sync
git submodule update --init --recursive
```

## Build the bootloader

To build the bootloader execute the following commands in the *config*-subdirectory:

```shell
make all
```

## Install the bootloader

Requirements:

- `avrdude`
- additional drivers for your programmer

To install the bootloader on the hardware connect it using a suitable programmer and
execute the following commands within the *config*-subdirectory:

```shell
make flash
```

You likely have to overwrite the `PROGRAMMER` variable with the required options for your programmer.
