#ifndef CHEDDAR_H
#define CHEDDAR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct GAPBuffer gap_buffer_t;
typedef struct ADDRBuffer addr_buffer_t;
typedef struct REGEXPBuffer regexp_buffer_t;
typedef struct WINBuffer win_buffer_t;
typedef struct TABBuffer tab_buffer_t;

struct TABBuffer {
  int tab_num;
  win_buffer_t *in_window;
  gap_buffer_t *txt_buffer;
  input_buffer_t *inp_buffer;
  output_buffer_t *outp_buffer const char32_t *title;
  bool vertical;
  struct TABBuffer *next;
  struct TABBuffer *children;
};

struct WINBuffer {
  int win_id;
  tab_buffer_t *tabs;
  size_t num_tabs;
  struct WINBuffer *next;
  struct WINBuffer *prev;
};

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

#endif
