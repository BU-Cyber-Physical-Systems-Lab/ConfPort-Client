# ConfPort Client

A simple client to interact with a generic configuration port, which will read
and write values by memory mapping `/dev/mem`.

**NOTE:** Big endian settings are implemented but have not been tested on a big endian machine.

## Configure the client

The configuration port client can be customised by modifying the content of
`confport_client.h`.

General characteristics of the configuration port are:
- `CONFPORT_BASE_ADDRESS`: The base address of the configuration port.
- `CONFPORT_SIZE_BYTE`: Total size of the configuration port, in bytes.
- `CONFPORT_REGS_NUM`: Number of registers in the configuration port.

Registers of the configuration port are represented as an array and identified
by their array index. `ADDR_OFFSET_ARR`, `REG_NAME_ARR` and `REG_SIZE_ARR` are
the arrays that hold information on the registers and a specific register is
represented by the information at a specific index of these arrays:
- `ADDR_OFFSET_ARR`: offset in bytes from the start of the configuration port.
- `REG_NAME_ARR`: Name of the register used to address the register as a long
  option.
- `REG_SIZE_ARR`: Size in bytes of the register.
- `REG_RDONLY_ARR`: If the register at a specified address is readonly (`1`) or read-write (`0`).


## Example configuration

The current configuration represents a configuration port with the current
properties:
- Little endianess defined the value of `LITTLE_ENDIAN`.
- A starting address of `0x80000000`, defined by the value of
  `CONFPORT_BASE_ADDRESS`.
- 5 registers, define by the value of `CONFPORT_REGS_NUM`.
- A total size of 48 bytes, defined by `CONFPORT_SIZE_BYTE`.
- All registers can be read and written as of `REG_RDONLY_ARR`.

Registers are currently configured in this way, according to `ADDR_OFFSET_ARR`,
`REG_NAME_ARR`, `REG_SIZE_ARR`:
- pgd : 8 bytes in size, starts at `0x80000000`.
- va : 8 bytes in size, starts at `0x80000008`.
- pa : 8 bytes in size, starts at `0x80000010`.
- offset : 8 bytes in size, starts at `0x80000028`.
- active : 1 bytes in size, starts at `0x80000018`.