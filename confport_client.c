#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "confport_client.h"

/// Then name of the registers as an array
static char *reg_names[CONFPORT_REGS_NUM] = REG_NAME_ARR;
/// The size of the registers in bytes
static size_t reg_sizes[CONFPORT_REGS_NUM] = REG_SIZE_ARR;
/// The `1` if the register is read only, `0` otherwise, as an array
static int reg_rdonly[CONFPORT_REGS_NUM] = REG_RDONLY_ARR;
/// The confport
static struct confport_t *confport;

// For the read operation we define a  macro that will loop over the bytes of a
// register in the right order,  according to the endianness of the target
// memory.

// While for the write operations we change the offset of the location where
// bytes will be written in the register according to the endianness of the
// target memory.
#if MEM_LITTLE_ENDIAN == 1
// @brief Loop logic for reading the bytes in little endian
#define CONFPORT_READ_LOOP()                                                   \
  int i = reg_sizes[index] - 1;                                                \
  i >= 0;                                                                      \
  i--
/* @brief Offset of the location where bytes will be written in the register for
 * little endian
 * @param[in] reg_index The index of the register.
 * @param[in] i The index of the byte, starting from the LSB
 */
#define WRITE_DISPLACEMENT(reg_index, i) i
#else
// @brief Loop logic for reading the bytes in big endian
#define CONFPORT_READ_LOOP()                                                   \
  int i = 0;                                                                   \
  i < reg_sizes[index];                                                        \
  i++
/* @brief Offset of the location where bytes will be written in the register for
 * big endian
 * @param[in] reg_index The index of the register.
 * @param[in] i The index of the byte, starting from the LSB
 */
#define WRITE_DISPLACEMENT(reg_index, i) reg_sizes[reg_index] - 1 - i
#endif

/// We then define the confport as a struct that contains pointers to each
/// of the registers.
typedef struct confport_t {
  int fd;                           ///< The file descriptor after the mmap.
  void *base_address;               ///< The base address of the confport.
  uint8_t *regs[CONFPORT_REGS_NUM]; ///< The registers of the confport.
} confport_t;

/*@brief mmap devmem to access the confport base
 * @param confport the base address of the confport.
 * @return a `struct confport_t*` on success,`NULL` on failure
 */
struct confport_t *map_registers() {
  struct confport_t *confport = malloc(sizeof(confport_t));
  unsigned sizes[CONFPORT_REGS_NUM] = ADDR_OFFSET_ARR;
  if (confport == NULL) {
    perror("cannot allocate memory for confport");
    return NULL;
  }
  confport->fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (confport->fd < 0) {
    perror("cannot open /dev/mem");
    free(confport);
    return NULL;
  }
  confport->base_address = mmap(
      NULL, (size_t)CONFPORT_SIZE_BYTE, PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_POPULATE, confport->fd, (off_t)CONFPORT_BASE_ADDRESS);
  if (confport->base_address == MAP_FAILED) {
    perror("cannot mmap /dev/mem");
    free(confport);
    return NULL;
  }
  for (int i = 0; i < CONFPORT_REGS_NUM; i++) {
    confport->regs[i] = (void *)(confport->base_address + sizes[i]);
  }
  mlockall(MCL_CURRENT | MCL_FUTURE);
  return confport;
}

///@brief unmap the confport base
void unmap_registers() {
  munlockall();
  munmap(confport->base_address, CONFPORT_SIZE_BYTE);
  close(confport->fd);
  free(confport);
}

/** @brief print the value of a register
 * @param[in] index The index of the register.
 */
static inline void print_register(int index) {
  printf("0x");
  for (CONFPORT_READ_LOOP()) {
    printf("%02x", *(confport->regs[index] + i));
  }
  printf("\n");
}

/**@brief write a confport register
 * @param[in] index The index of the register.
 * @param[in] value The value to write.
 * @return 0 on success, -1 on failure
 * @details The value is written in the register in the right order, according
 * to the endianness of the target memory.
 * The function also makes sure that the value is a valid hexadecimal number
 * with an even number of characters and its size in bytes can be contained by
 * the selected register. If the value is less than the size of the register,
 * the function pads with 0s.
 * Funtion will fail is the register is readonly.
 */
static inline int write_register(int index, char *value) {
  if (reg_rdonly[index] == 1) {
    printf("Error while writing in %s\n", reg_names[index]);
    printf("Register is read only\n");
    return -1;
  }
  // we get rid of  '0x' and '0X' if present
  char *to_write = (value[0] == '0' && (value[1] == 'x' || value[1] == 'X'))
                       ? value + 2
                       : value;
  // check if all characters are hexadecimal
  for (int i = 0; i < strlen(to_write); i++) {
    if (!((to_write[i] >= '0' && to_write[i] <= '9') ||
          (to_write[i] >= 'a' && to_write[i] <= 'f') ||
          (to_write[i] >= 'A' && to_write[i] >= 'F'))) {
      printf("Error while writing in %s\n", reg_names[index]);
      printf("Invalid character %c in value 0x%s\n", to_write[i], to_write);
      return -1;
    }
  }
  // each two char are a byte in hex (eg. 6A is a byte, 6A6A is two bytes)
  int res = 0, value_size = strlen(to_write);
  char *tmp = NULL;
  // make sure we have an even number of characters!
  if (value_size % 2 != 0) {
    value_size++;
    tmp = malloc(value_size + 1);
    if (tmp == NULL) {
      printf("Error while writing in %s\n", reg_names[index]);
      perror("cannot allocate memory for non even write value");
      return -1;
    }
    tmp[0] = '0';
    tmp[1] = '\0';
    strcat(tmp, to_write);
    to_write = tmp;
  }
  // we divide by two to get the number of bytes
  value_size = value_size / 2;
  char byte_to_write_str[3] = {'0', '0', '\0'};
  uint8_t byte_to_write;
  int char_index = 0;
  if (value_size <= reg_sizes[index]) {
    // we start from the MSBs to the LSBs one byte at a time
    for (int i = reg_sizes[index] - 1; i >= 0; i--) {
      // if we we have less characters than the size of the register we pad
      // with
      // 0
      if (i < value_size) {
        char_index = value_size - i - 1;
        byte_to_write_str[0] = to_write[2 * char_index];
        byte_to_write_str[1] = to_write[1 + (2 * char_index)];
      }
      byte_to_write =
          (uint8_t)strtol((const char *)&byte_to_write_str, NULL, 16);
      if (errno != 0) {
        res = -1;
        printf("Error while writing in %s\n", reg_names[index]);
        printf("Failed to convert %s to a byte\n", byte_to_write_str);
        perror("invalid value");
        if (tmp != NULL) {
          free(tmp);
        }
        return res;
      }
      memcpy(confport->regs[index] + WRITE_DISPLACEMENT(index, i),
             (uint8_t *)&byte_to_write, sizeof(uint8_t));
    }
  } else {
    res = -1;
    printf("Error while writing in %s\n", reg_names[index]);
    printf("Error: size of value (%d bytes) is greater than the size of the "
           "register %s (%ld bytes)\n",
           value_size, reg_names[index], reg_sizes[index]);
  }
  if (tmp != NULL) {
    free(tmp);
  }
  return res;
}

///@brief print the help message
///@param[in] name the name of the program
static inline void help(char *name) {
  printf("Usage: %s [OPTION]...\n", name);
  printf("If the option can take an argument, and the argument is given, \n"
         "that value will be written in the "
         "corresponding register.\n"
         "Otherwise the register will be read.\n");
  printf("If no option is provided all registers will be read.\n");
  printf("Options:\n");
  for (int i = 0; i < CONFPORT_REGS_NUM; i++) {
    if (reg_rdonly[i] == 0) {
      printf("\t --%s [value]\n", reg_names[i]);
    } else {
      printf("\t --%s \n", reg_names[i]);
    }
  }
}

int main(int argc, char **argv) {
  static struct option long_options[CONFPORT_REGS_NUM + 2];
  int res = 0, empty = 1, opt;
  confport = map_registers();
  if (confport == NULL) {
    return -1;
  }
  for (int i = 0; i < CONFPORT_REGS_NUM; i++) {
    long_options[i].name = reg_names[i];
    long_options[i].has_arg =
        (reg_rdonly[i] == 0) ? optional_argument : no_argument;
    long_options[i].flag = NULL;
    long_options[i].val = i;
  }
  long_options[CONFPORT_REGS_NUM].name = "help";
  long_options[CONFPORT_REGS_NUM].has_arg = no_argument;
  long_options[CONFPORT_REGS_NUM].flag = NULL;
  long_options[CONFPORT_REGS_NUM].val = 'h';
  long_options[CONFPORT_REGS_NUM + 1].name = 0;
  long_options[CONFPORT_REGS_NUM + 1].has_arg = 0;
  long_options[CONFPORT_REGS_NUM + 1].flag = 0;
  long_options[CONFPORT_REGS_NUM + 1].val = 0;
  while (res == 0 &&
         (opt = getopt_long(argc, argv, "-h", long_options, NULL)) != -1) {
    switch (opt) {
    case 'h':
      empty = 0;
      help(argv[0]);
      break;
    case '?':
      empty = 0;
      res = -1;
      printf("Error :Unknown option\n");
      break;
    default:
      empty = 0;
      // https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
      // optarg_long ignores options in the format --foo bar with optional
      // arguments
      if (optarg == NULL && optind < argc && argv[optind - 1][0] == '-') {
        optarg = argv[optind];
        optind++;
      }
      if (optarg == NULL) {
        print_register(opt);
      } else {
        res = write_register(opt, optarg);
      }
      break;
    }
  }
  if (res == -1) {
    printf("\n");
    help(argv[0]);
  } else {
    // if no option is provided we print all registers
    if (empty) {
      for (int i = 0; i < CONFPORT_REGS_NUM; i++) {
        printf("%s: ", reg_names[i]);
        print_register(i);
      }
    }
  }
  unmap_registers();
  return res;
}
