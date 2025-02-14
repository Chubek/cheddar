#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#define BUFFER_GROWTH_RATE 1.2

extern Arena *current_arena;

typedef uint32_t regex_flag_t, hash_t;
typedef struct STRBuffer str_buffer_t;
typedef struct LINEBuffer line_buffer_t;
typedef struct TXTBuffer txt_buffer_t;
typedef struct REBuffer regex_buffer_t;
typedef struct INBuffer input_buffer_t;
typedef struct OUTBuffer output_buffer_t;
typedef struct MARKBuffer mark_buffer_t;
typedef struct CMDBuffer cmd_buffer_t;
typedef struct Address address_t;
typedef struct TXTBufferPair txtbuf_pair_t;

struct Address {
  enum AddressKind {
    ADDR_AbsNumber,
    ADDR_RelNumber,
    ADDR_Start,
    ADDR_End,
    ADDR_PattNext,
    ADDR_PattPrev,
    ADDR_Mark,
  } kind;

  union {
    ssize_t v_number;
    regex_buffer_t *v_patt;
    mark_buffer_t *v_mark;
  };
};

struct STRBuffer {
  char32_t *contents;
  size_t length;
  size_t capacity;
  struct STRBuffer *next;
};

struct LINEBuffer {
  str_buffer_t *contents;
  size_t line_no;
  struct LINEBuffer *next;
  struct LINEBuffer *prev;
};

struct TXTBuffer {
  line_buffer_t *lines;
  size_t num_lines;
  size_t line_capacity;
};

struct REBuffer {
  str_buffer_t *patt;
  str_buffer_t *subst;
  regex_flag_t flags;
};

struct INBuffer {
  address_t addr_start;
  address_t addr_end;
  cmd_buffer_t *command;
};

struct OUTBuffer {
  txt_buffer_t *source_text;
  str_buffer_t *scratch;
  size_t line_start;
  size_t line_end;
  FILE *outfile;
  bool force;
};

struct MARKBuffer {
  size_t line_no;
  str_buffer_t *name;
  hash_t name_hash;
  struct MARKBuffer *next;
};

struct CMDBuffer {
  enum CMDKind {
    CMD_Find,
    CMD_FindReplace,
    CMD_Output,
    CMD_Exec,
    CMD_Append,
    CMD_Insert,
    CMD_Mark,
    // ADD MORE TODO
  } cmd_kind;
};

struct TXTBufferPair {
  txt_buffer_t *left;
  txt_buffer_t *right;
};

str_buffer_t *str_buffer_new_blank(size_t capacity) {
  str_buffer_t *buffer = request_memory(current_arena, sizeof(str_buffer_t));
  buffer->contents = request_memory(current_arena, capacity * sizeof(char32_t));
  buffer->capacity = capacity;
  buffer->length = 0;
  buffer->next = NULL;

  return buffer;
}

str_buffer_t *str_buffer_new_from(char32_t *contents, size_t length) {
  str_buffer_t *buffer = str_buffer_new_blank(length * BUFFER_GROWTH_RATE);
  str_buffer_concat(buffer, contents, length);
  return buffer;
}

str_buffer_t *str_buffer_grow_capacity(str_buffer_t *buffer,
                                       size_t minimum_growth) {
  size_t growth_size = (buffer->capacity + minimum_growth) * BUFFER_GROWTH_RATE;
  TO_NEAREST_POWER_OF_TWO(growth_rate);

  buffer->contents =
      resize_memory(&current_arena, buffer->contents, buffer->capacity,
                    growth_size, sizeof(char32_t));
  buffer->capacity = growth_size;

  return buffer;
}

str_buffer_t *str_buffer_copy(str_buffer_t *buffer, char32_t *copy_contents,
                              size_t start, size_t span) {
  if (span - start >= buffer->capacity - buffer->length)
    buffer = str_buffer_grow_capacity(buffer, span - start);

  memmove(&buffer->contents[start], copy_contents, span * sizeof(char32_t));
  buffer->length += span - start;

  return buffer;
}

str_buffer_t *str_buffer_concat(str_buffer_t *buffer, char32_t *concat_contents,
                                size_t concat_length) {
  if (concat_length >= buffer->capacity - buffer->length)
    buffer = str_buffer_grow_capacity(buffer, concat_length);

  memmove(&buffer->contents[buffer->length - 1], concat_contents,
          concat_length * sizeof(char32_t));
  buffer->length += concat_length;

  return buffer;
}

str_buffer_t *str_buffer_merge(str_buffer_t *buffer_a, str_buffer_t *buffer_b) {
  if (buffer_a == NULL || buffer_b == NULL)
    return NULL;

  str_buffer_t *merged =
      str_buffer_new_blank(buffer_a->length + buffer_b->length);
  memmove(merged->contents, buffer_a->contents,
          buffer_a->length * sizeof(char32_t));
  memmove(&merged->contents[buffer_a->length - 1], buffer_b->contents,
          buffer_b->length * sizeof(char32_t));

  return merged;
}

str_buffer_t *str_buffer_add_char(str_buffer_t *buffer, char32_t chr) {
  if (buffer->length + 1 >= buffer->capacity)
    buffer = str_buffer_grow_capacity(buffer, 0);

  buffer->contents[buffer->length++] = chr;

  return buffer;
}

str_buffer_t *str_buffer_new_substring(str_buffer_t *buffer, size_t start,
                                       size_t span) {
  if (buffer == NULL)
    return NULL;

  str_buffer_t *result = str_buffer_new_blank(span);
  result = str_buffer_copy(result, buffer->contents, start, span);
  return result;
}

str_buffer_t *str_buffer_list_append(str_buffer_t **list,
                                     str_buffer_t *buffer) {
  if (list == NULL || *list == NULL) {
    *list = buffer;
    return *list;
  }

  str_buffer_t *head = *list;

  while (head->next != NULL)
    head = head->next;

  head->next = buffer;

  return head;
}

str_buffer_t *str_buffer_splice_char(str_buffer_t *buffer, size_t index_left,
                                     size_t index_right, char32_t chr) {
  if (buffer->length + 1 <= buffer->capacity)
    buffer = str_buffer_grow_capacity(buffer, index_right - index_left);

  const char32_t *backup =
      strndup(buffer->contents, buffer->length * sizeof(char32_t));
  memset(buffer->contents, 0, buffer->length * sizeof(char32_t));
  memmove(buffer->contents, backup, index_left * sizeof(char32_t));
  memmove(&buffer->contents[index_right], &backup[index_right],
          (buffer->length - index_right) * sizeof(char32_t));
  memset(&buffer->contents[index_left + 1], chr,
         (index_right - index_left) * sizeof(char32_t));
  free(backup);

  return buffer;
}

str_buffer_t *str_buffer_splice_substring(str_buffer_t *buffer,
                                          str_buffer_t *substring,
                                          size_t insert_pos) {
  if (buffer->length + substring->length <= buffer->capacity)
    buffer = str_buffer_grow_capacity(buffer, substring->length);

  const char32_t *backup =
      strndup(buffer->contnents, buffer->length * sizeof(char32_t));
  memset(buffer->contents, 0, buffer->length * sizeof(char32_t));
  memmove(buffer->contents, backup, insert_pos * sizeof(char32_t));
  memmove(&buffer->contents[insert_pos - 1], substring->contents,
          substring->length * sizeof(char32_t));
  memmove(&buffer->contents[insert_pos + substring->length],
          &backup[insert_pos],
          (buffer->length - insert_pos) * sizeof(char32_t));
  free(backup);

  return buffer;
}

str_buffer_t *str_buffer_remove_chunk(str_buffer_t *buffer, size_t start,
                                      size_t span) {
  if (buffer->length <= span)
    return NULL;

  const char32_t *backup =
      strndup(buffer->contents, buffer->length * sizeof(char32_t));
  memset(&buffer->contents[start], 0, span * sizeof(char32_t));
  memmove(&buffer->contents[span - start], &buffer->contents[start + 1],
          (buffer->length - span) * sizeof(char32_t));

  return buffer;
}

hash_t str_buffer_hash(str_buffer_t *buffer) {
  if (buffer == NULL)
    return 0;

  hash_t hash = 5381;

  for (size_t i = 0; i < buffer->length; i++)
    hash = (hash * 33) + buffer->contents[i];

  return hash;
}

line_buffer_t *line_buffer_new(str_buffer_t *contents, size_t line_no) {
  line_buffer_t *buffer = request_memory(current_arena, sizeof(line_buffer_t));
  buffer->contents = contents;
  buffer->line_no = line_no;
  buffer->next = NULL;
  buffer->prev = NULL;
}

line_buffer_t *line_buffer_list_append(line_buffer_t **list,
                                       line_buffer_t *buffer) {
  if (list == NULL || *list == NULL) {
    *list = buffer;
    return *list;
  }

  line_buffer_t *head = *list;

  while (head->next != NULL)
    head = head->next;

  buffer->prev = head;
  head->next = buffer;

  return head;
}

line_buffer_t *line_buffer_list_prepend(line_buffer_t **list,
                                        line_buffer_t *buffer) {
  if (list == NULL || *line == NULL) {
    *list = buffer;
    return *list;
  }

  line_buffer_t *tail = *list;

  while (tail->prev != NULL)
    tail = tail->prev;

  buffer->next = tail;
  tail->prev = buffer;

  return tail;
}

txt_buffer_t *txt_buffer_new(size_t capacity) {
  txt_buffer_t *buffer = request_memory(current_arena, sizeof(txt_buffer_t));
  buffer->lines =
      request_memory(current_arena, sizeof(line_buffer_t) * capacity);
  buffer->capacity = capacity;
  buffer->num_lines = 0;
  return buffer;
}

txt_buffer_t *txt_buffer_grow_capacity(txt_buffer_t *buffer,
                                       size_t minimum_growth) {
  size_t growth_size = (buffer->capacity + minimum_growth) * BUFFER_GROWTH_RATE;
  TO_NEAREST_POWER_OF_TWO(growth_size);

  buffer->lines = resize_memory(&current_arena, buffer->lines, buffer->capacity,
                                growth_size, sizeof(line_buffer_t));
  buffer->capacity = growth_size;

  return buffer;
}

txt_buffer_t *txt_buffer_insert_line(txt_buffer_t *buffer,
                                     line_buffer_t *line) {
  if (buffer->num_lines + 1 >= buffer->capacity)
    txt_buffer_grow_capacity(buffer, 1);
  buffer->lines[buffer->num_lines++] = line;
  return buffer;
}

line_buffer_t *txt_buffer_get_nth_line(txt_buffer_t *buffer, size_t index) {
  if (index >= buffer->num_lines)
    return NULL;
  return buffer->lines[index];
}

line_buffer_t *txt_buffer_get_line_range(txt_buffer_t *buffer, size_t start,
                                         size_t end) {
  if (end >= buffer->num_lines)
    return NULL;

  line_buffer_t *result = NULL;
  for (size_t i = start; i < end; i++)
    result = line_buffer_list_append(&result, buffer->lines[i]);

  return result;
}

line_buffer_t *txt_buffer_get_line_range_reverse(txt_buffer_t *buffer,
                                                 size_t end, size_t start) {
  if (end >= buffer->num_lines)
    return NULL;

  line_buffer_t *result = NULL;
  for (size_t i = end; i >= start; i++)
    result = line_buffer_list_prepend(&result, buffer->lines[i]);

  return result;
}

regex_buffer_t *regex_buffer_create(str_buffer_t *patt, str_buffer_t *subst,
                                    regex_flags_t flags) {
  regex_buffer_t *buffer =
      request_memory(current_arena, sizeof(regex_buffer_t));
  buffer->patt = patt;
  buffer->subst = subst;
  buffer->flags = flags;
  return buffer;
}

mark_buffer_t *mark_buffer_insert(mark_buffer_t **table, size_t line_no,
                                  str_buffer_t *name) {
  hash_t name_hash = str_buffer_hash(name);
  mark_buffer_t *head = *table;

  while (head->next != NULL) {
    if (head->name_hash == name_hash)
      break;
    head = head->next;
  }

  if (head->next == NULL) {
    head->next = request_memory(current_arena, sizeof(mark_buffer_t));
    head = head->next;
    head->next = NULL;
  }

  head->line_no = line_no;
  head->name = name;
  head->name_hash = name_hash;

  return head;
}

in_buffer_t *in_buffer_new(address_t *addr_start, address_t *addr_end,
                           cmd_buffer_t *command) {
  in_buffer_t *buffer = request_memory(current_arena, sizeof(in_buffer_t));

  buffer->addr_start = addr_start;
  buffer->addr_end = addr_end;
  buffer->command = command;

  return buffer;
}

out_buffer_t *out_buffer_new(/* TODO */) {
  // TODO
}
