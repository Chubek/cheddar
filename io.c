#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <uchar.h>
#include <unistd.h>

#define VALIDATE_U32_RANGE(chr)                                                \
  ((chr >= 0 && chr <= 0x10FFFF) && !(chr >= 0xD800 && chr <= 0xDFFF))
#define VALIDATE_U32_ENDIANNESS(bom) (bom == 0x0000FEFF)

static struct termios original_terminal_settings;

void save_original_settings(void) {
  if (tcgetattr(STDIN_FILENO, &original_terminal_settings) != 0)
    errno_raise("tcgetattr");
}

void restore_original_settings(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_settings) != 0)
    errno_raise("tcsetattr");
}

void setup_terminal(void) {
  save_original_settings();
  atexit(restore_original_settings);

  struct termios raw = original_terminal_settings;

  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_lflag &= ~ISIG;
  raw.c_iflag &= ~(ICRNL | IXON);
  raw.c_oflag &= ~OPOST;

  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0)
    errno_raise("tcsetattr");
}

char32_t read_u32_character(static bool *is_big_endian) {
  static char32_t chr = -1;

  if (chr == -1) {
    if (read(STDIN_FILENO, sizeof(char32_t), &chr) != sizeof(char32_t))
      return -1;

    if (chr != 0x0000FEFF || chr != 0xFFFE0000)
      return chr;

    *is_big_endian = VALIDATE_U32_ENDIANNESS(chr);
  }

  if (read(STDIN_FILENO, sizeof(char32_t), &chr) != sizeof(char32_t))
    return -1;

  if (!VALIDATE_U32_RANGE(chr))
    errno_raise("Invalid character input");

  return chr;
}

void convert_u32_to_byte_sequence(char32_t chr, uint8_t byte_seq[static 4],
                                  bool is_big_endian) {
  if (is_big_endian) {
    byte_seq[0] = (chr >> 24) & 0xFF;
    byte_seq[1] = (chr >> 16) & 0xFF;
    byte_seq[2] = (chr >> 8) & 0xFF;
    byte_seq[3] = chr & 0xFF;
  } else {
    byte_seq[3] = (chr >> 24) & 0xFF;
    byte_seq[2] = (chr >> 16) & 0xFF;
    byte_seq[1] = (chr >> 8) & 0xFF;
    byte_seq[0] = chr & 0xFF;
  }
}
