#ifndef ADDRESSES_H
#define ADDRESSES_H

/// Base address of the configuration port
#define CONFPORT_BASE_ADDRESS 0x80000000
/// The size of the configuration port in bytes
#define CONFPORT_SIZE_BYTE 48
/// The number of registers in the configuration port
#define CONFPORT_REGS_NUM 5
/// registers sizes in bytes (hexadecimal) in id order
#define REG_SIZE_ARR                                                           \
  { 8, 8, 8, 8, 1 }
/// registers offsets in bytes (hexadecimal) in id order
#define ADDR_OFFSET_ARR                                                        \
  { 0x0, 0x8, 0x10, 0x28, 0x18 }
/// index to name mapping for the confport registers
#define REG_NAME_ARR                                                           \
  { "pgd", "va", "pa", "offset", "active" }
/// If the target memory is little endian
#define MEM_LITTLE_ENDIAN 1
/// `0` if register is can be written to, `1` if register is read only
#define REG_RDONLY_ARR                                                           \
  { 0, 0, 0, 0, 0 }
#endif
