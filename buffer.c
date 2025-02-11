#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

typedef uint32_t regex_flag_t;
typedef struct TXTBuffer txt_buffer_t;
typedef struct REBuffer re_buffer_t;
typedef struct INBuffer input_buffer_t;
typedef struct OUTBuffer output_buffer_t;
typedef struct Address address_t;

struct Address {
  enum AddressKind {
    ADDR_AbsNumber,
    ADDR_RelNumber,
    ADDR_Start,
    ADDR_End,
    ADDR_PrevLn,
    ADDR_NextLn,
  } kind;

  size_t value;
};

struct TXTBuffer {
  char32_t *contents;
  size_t length;
  struct TXTBuffer *next;
  struct TXTBuffer *prev;
};

struct REBuffer {
  char32_t *patt;
  char32_t *subst;
  size_t patt_length;
  size_t subst_length;
  regex_flag_t flags;
};

struct INBuffer {
  address_t addr_start;
  address_t addr_end;
  command_t *command;
};

struct OUTBuffer {
  txt_buffer_t *source_text;
  size_t pos_start;
  size_t pos_end;
  FILE *outfile;
  bool force;
};
