#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct CMDLineInsert cmd_line_insert_t;
typedef struct CMDSpliceChar cmd_splice_char_t;
typedef struct CMDSpliceString cmd_splice_string_t;
typedef struct CMDDeleteChunk cmd_delete_chunk_t;

struct CMDLineInsert {
  txt_buffer_t *buffer;
  line_buffer_t *line;
};

struct CMDSpliceChar {
  txt_buffer_t *buffer;
  char32_t chr;
  size_t line_no;
  size_t at_pos;
};

struct CMDSpliceString {
  txt_buffer_t *buffer;
  str_buffer_t *string;
  size_t line_no;
  size_t index;
};

struct CMDDeleteChunk {
  txt_buffer_t *buffer;
  size_t line_no;
  size_t start;
  size_t span;
};
