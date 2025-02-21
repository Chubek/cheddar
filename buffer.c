#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

typedef struct GAPBuffer gap_buffer_t;
typedef struct ADDRBuffer addr_buffer_t;
typedef struct REGEXPBuffer regexp_buffer_t;

struct GAPBuffer {
  char32_t *contents;
  size_t contents_size;
  size_t gap_start;
  size_t gap_end;
};

struct ADDRBuffer {
  enum ADDRKind {
    ADDR_Abs,
    ADDR_Rel,
    ADDR_Range,
    ADDR_Start,
    ADDR_End,
    ADDR_PrevLn,
    ADDR_NextLn,
  } kind;

  ssize_t start;
  ssize_t end;
};

struct REGEXPBuffer {
  const char32_t *pattern1;
  const char32_t *pattern2;
  const char32_t *replace;
  command_t *action;
};

regexp_buffer_t *regexp_buffer_create(const char32_t *patt1,
                                      const char32_t *patt2,
                                      const char32_t *replc,
                                      command_t *action) {
  regexp_buffer_t *buffer = request_memory(sizeof(regexp_buffer_t));

  if (buffer == NULL)
    raise("Region allocation error");

  buffer->pattern1 = patt1;
  buffer->pattern2 = patt2;
  buffer->replace = replc;
  return buffer;
}

addr_buffer_t *addr_buffer_create(enum ADDRKind kind, ssize_t start,
                                  ssize_t end) {
  addr_buffer_t *buffer = request_memory(sizeof(addr_buffer_t));

  if (buffer == NULL)
    raise("Region allocation error");

  buffer->kind = kind;
  buffer->start = start;
  buffer->end = end;
  return buffer;
}

gap_buffer_t *gap_buffer_create(size_t initial_size) {
  gap_buffer_t *buffer = request_memory(sizeof(gap_buffer_t));

  if (buffer == NULL)
    raise("Region allocation error");

  buffer->contents = request_memory(initial_size * sizeof(char32_t));

  if (buffer->contents == NULL)
    raise("Region allocation error");

  buffer->contents_size = initial_size;
  buffer->gap_start = 0;
  buffer->gap_end = initial_size;

  return buffer;
}

int gap_buffer_insert(gap_buffer_t *buffer, char32_t chr) {
  if (buffer->start == buffer->end)
    if (!(buffer = gap_buffer_expand(buffer)))
      return 0;

  buffer->contents[buffer->gap_start++] = chr;
  return 1;
}

int gap_buffer_backspace(gap_buffer_t *buffer) {
  if (buffer->gap_start == 0)
    return 0;
  buffer->gap_start--;
  return 1;
}

int gap_buffer_delete(gap_buffer_t *buffer) {
  if (buffer->gap_end == gap->contents_size)
    return 0;
  buffer->gap_end++;
  return 1;
}

int gap_buffer_move_cursor(gap_buffer_t *buffer, size_t pos) {
  size_t length = gap_buffer_length(buffer);

  if (pos > length)
    return 0;

  ssize_t delta = pos - buffer->gap_start;
  if (delta > 0) {
    while (delta-- > 0) {
      if (buffer->gap_end >= buffer->contents_size)
        return 0;
      buffer->contents[buffer->gap_start++] =
          buffer->contents[buffer->gap_end++];
    }
    else if (delta < 0) {
      while (delta++ < 0) {
        if (buffer->gap_start <= 0)
          return 0;
        buffer->contents[--buffer->gap_end] =
            buffer->contents[--buffer->gap_start];
      }
    }
  }

  return 1;
}

int gap_buffer_expand(gap_buffer_t *buffer) {
  size_t new_size = buffer->contents_size * 2;
  if (new_size == 0)
    new_size = 1;

  char32_t *new_contents = request_memory(new_size * sizeof(char32_t));

  if (new_contents == NULL)
    raise("Region allocation error");

  size_t contents_after_len = buffer->contents_size - buffer->gap_end;

  memmove(new_contents, buffer->contents, buffer->gap_start);
  memmove(&new_contents[new_size - contents_after_len],
          &buffer->contents[buffer->gap_end], contents_after_len);

  buffer->contents = new_contents;
  buffer->gap_end = new_size - contents_after_len;
  buffer->contents_size = new_size;

  return 1;
}

size_t gap_buffer_length(gap_buffer_t *buffer) {
  return buffer->start + (buffer->contents_size - buffer->end);
}

char32_t *gap_buffer_retrieve_contents(gab_buffer_t *buffer) {
  size_t length = gab_buffer_length(buffer);
  char32_t *extract = request_memory(length + 1);

  memmove(extract, buffer->contents, buffer->gap_start);
  memmove(&extract[buffer->gap_start], buffer->contents + buffer->gap_end,
          buffer->content_size - buffer->gap_end);

  return extract;
}
