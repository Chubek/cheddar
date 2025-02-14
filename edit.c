#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_BUFFER_INIT_CAP 1024

extern Arena *current_arena;

static bool is_big_endian = false;
static txt_buffer_t *read_buffer = NULL;

void read_line(void) {
  str_buffer_t *line_buffer = str_buffer_new_blank(STR_BUFFER_INIT_CAP);
  char32_t read_char = 0;

  while (read_char != '\n') {
    read_char = read_u32_character(&is_big_endian);
    if (read_char == -1)
      return NULL;
    line_buffer = str_buffer_add_char(line_buffer, read_char);
  }

  return line_buffer;
}

void read_to_buffer(void) {
   txt_buffer_t *buffer = NULL;
   str_buffer_t *curr_ln = NULL;

   while (true) {
	curr_ln = read_line();
	if (curr_ln == NULL)
		break;

	buffer = txt_buffer_new_leaf(curr_ln);
	read_buffer = txt_buffer_new_internal(buffer, read_buffer);
   }
}
