#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
  } cmd_kind;

  union {
    cmd_line_inert_t v_line_insert;
    cmd_splice_char_t v_splice_char;
    cmd_splice_strig_t v_splice_string;
    cmd_delete_chunk_t v_delete_chunk;
    // TODO: Add more
  };
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
