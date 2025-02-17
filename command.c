#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern Arena *current_arena;

typedef struct Command command_t;
typedef struct CMDLineInsert cmd_line_insert_t;
typedef struct CMDSpliceChar cmd_splice_char_t;
typedef struct CMDSpliceString cmd_splice_string_t;
typedef struct CMDDeleteChunk cmd_delete_chunk_t;

struct Command {
  enum CMDKind {
    CMD_LineInsert,
    CMD_SpliceChar,
    CMD_SpliceString,
    CMD_DeleteChunk,
    CMD_Undo,
    CMD_Redo,
  } cmd_kind;

  union {
    cmd_line_inert_t v_line_insert;
    cmd_splice_char_t v_splice_char;
    cmd_splice_strig_t v_splice_string;
    cmd_delete_chunk_t v_delete_chunk;
    struct Command *v_command_list;
    // TODO: Add more
  };

  struct Command *next;
  struct Command *prev;
};

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

command_t *push_command(command_t **list, command_t *cmd) {
  if (list == NULL || *list == NULL) {
    *list = cmd;
    return *list;
  }

  command_t *head = *list;

  while (head->next != NULL)
    head = head->next;

  cmd->prev = head;
  head->next = cmd;

  return head;
}

command_t *pop_command(command_t **list) {
  if (list == NULL || *list == NULL)
    return NULL;

  command_t *head = *list;

  while (head->next != NULL)
    head = head->next;

  if (head->prev != NULL) {
    head->prev->next = NULL;
    head->prev = NULL;
  }

  return head;
}

command_t *command_new_insert_line(txt_buffer_t *buffer, line_buffer_t *line) {
  command_t *cmd = request_memory(current_arena, sizeof(command_t));
  cmd->cmd_kind = CMD_InsertLine;
  cmd->v_insert_line.buffer = buffer;
  cmd->v_insert_line.line = line;
  return cmd;
}

command_t *command_new_splice_char(txt_buffer_t *buffer, char32_t chr,
                                   size_t line_no, size_t at_pos) {
  command_t *cmd = request_memory(current_arena, sizeof(command_t));
  cmd->cmd_kind = CMD_SpliceChar;
  cmd->v_splice_char.buffer = buffer;
  cmd->v_splice_char.chr = chr;
  cmd->v_splice_char.line_no = line_no;
  cmd->v_splice_char.at_pos = at_pos;
  return cmd;
}

command_t *command_new_splice_string(txt_buffer_t *buffer, str_buffer_t *string,
                                     size_t line_no, size_t index) {
  command_t *cmd = request_memory(current_arena, sizeof(command_t));
  cmd->cmd_kind = CMD_SpliceString;
  cmd->v_splice_string.buffer = buffer;
  cmd->v_splice_string.string = string;
  cmd->v_splice_string.line_no = line_no;
  cmd->v_splice_string.index = index;
  return cmd;
}

command_t *command_new_delete_chunk(txt_buffer_t *buffer, size_t line_no,
                                    size_t start, size_t span) {
  command_t *cmd = request_memory(current_arena, sizeof(command_t));
  cmd->cmd_kind = CMD_DeleteChunk;
  cmd->v_delete_chunk.buffer = buffer;
  cmd->v_delete_chunk.line_no = line_no;
  cmd->v_delete_chunk.start = start;
  cmd->v_delete_chunk.span = span;
  return cmd;
}

command_t *command_new_undo(void) {
  command_t *cmd = request_memory(current_arena, sizeof(command_t));
  cmd->cmd_kind = CMD_Undo;
  cmd->v_command_list = NULL;
  return cmd;
}

command_t *command_new_redo(void) {
  command_t *cmd = request_memory(current_arena, sizeof(command_t));
  cmd->cmd_kind = CMD_Redo;
  cmd->v_command_list = NULL;
  return cmd;
}
