#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_BUFFER_INIT_CAP 1024
#define TXT_BUFFER_INIT_CAP 512

extern Arena *current_arena;

static size_t line_number = 0;
static bool is_big_endian = false;

line_buffer_t *read_line(void) {
  str_buffer_t *line_buffer = str_buffer_new_blank(LINE_BUFFER_INIT_CAP);
  char32_t read_char = 0;

  while (read_char != '\n') {
    read_char = read_u32_character(&is_big_endian);
    if (read_char == -1)
      return NULL;
    line_buffer = str_buffer_add_char(line_buffer, read_char);
  }

  return line_buffer_new(line_buffer, ++line_number);
}

txt_buffer_t *read_lines_to_text_buffer(void) {
  line_buffer_t *curr_line = NULL;
  txt_buffer_t *text_buffer = txt_buffer_new_blank(TXT_BUFFER_INIT_CAP);

  while (true) {
    curr_line = read_line();
    if (curr_line == NULL)
      break;

    text_buffer = txt_buffer_insert_line(text_buffer, curr_line);
  }

  return text_buffer;
}

txt_buffer_t *insert_substring_at_nth_line
