#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#define BUFFER_GROWTH_RATE 1.2

extern Arena *current_arena;

typedef uint32_t regex_flag_t;
typedef struct STRBuffer str_buffer_t;
typedef struct TXTBuffer txt_buffer_t;
typedef struct REBuffer regex_buffer_t;
typedef struct INBuffer input_buffer_t;
typedef struct OUTBuffer output_buffer_t;
typedef struct Address address_t;
typedef struct TXTBufferPair txtbuf_pair_t;

struct Address {
  enum AddressKind {
    ADDR_AbsNumber,
    ADDR_RelNumber,
    ADDR_Start,
    ADDR_End,
    ADDR_PrevLn,
    ADDR_NextLn,
  } kind;

  ssize_t value;
};

struct STRBuffer {
  char32_t *contents;
  size_t length;
  size_t capacity;
  struct STRBuffer *next;
};

struct TXTBuffer {
  enum RopeKind {
    ROPE_Internal,
    ROPE_Leaf,
  } rope_kind;
  size_t weight;

  union {
    struct {
      struct TXTBuffer *left;
      struct TXTBuffer *right;
    } internal;

    str_buffer_t *leaf;
  };
};

struct REBuffer {
  str_buffer_t *patt;
  str_buffer_t *subst;
  regex_flag_t flags;
};

struct INBuffer {
  address_t addr_start;
  address_t addr_end;
  command_t *command;
};

struct OUTBuffer {
  txt_buffer_t *source_text;
  str_buffer_t *scratch;
  size_t line_start;
  size_t line_end;
  FILE *outfile;
  bool force;
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

  memmove(&buffer->contents[start - 1], copy_contents, span * sizeof(char32_t));
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

str_buffer_t *str_buffer_add_char(str_buffer_t *buffer, char32_t chr) {
  if (buffer->length + 1 >= buffer->capacity)
    buffer = str_buffer_grow_capacity(buffer, 0);

  buffer->contents[buffer->length] = chr;

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

txt_buffer_t *txt_buffer_new_leaf(str_buffer_t *leaf) {
  txt_buffer_t *node = request_memory(current_arena, sizeof(txt_buffer_t));
  node->rope_kind = ROPE_Leaf;
  node->leaf = leaf;
  node->weight = leaf->length;

  return node;
}

txt_buffer_t *txt_buffer_new_internal(txt_buffer_t *left, txt_buffer_t *right) {
  txt_buffer_t *node = request_memory(current_arena, sizeof(txt_buffer_t));
  node->rope_kind = ROPE_Internal;
  node->internal.left = left;
  node->internal.right = right;
  node->weight =
      (left->rope_kind == ROPE_Leaf) ? left->leaf->length : left->weight;

  return node;
}

void txt_buffer_apply(const txt_buffer_t *node,
                      void (*apply_fn)(const str_buffer_t *)) {
  if (node == NULL)
    return;

  if (node->rope_kind == ROPE_Leaf)
    apply_fn(node->leaf);
  else {
    txt_buffer_apply(node->internal.left, apply_fn);
    txt_buffer_apply(node->internal.right, apply_fn);
  }
}

char32_t txt_buffer_index(const txt_buffer_t *node, size_t index) {
  if (node->rope_kind == ROPE_Leaf) {
    if (index < node->leaf->length)
      return node->leaf->contents[index];
    else
      return U'\0';
  } else {
    if (index < node->weight)
      return txt_buffer_index(node->internal.left, index);
    else
      return txt_buffer_index(node->internal.right, index - node->weight);
  }

  return U'\0';
}

str_buffer_t *txt_buffer_extract_range(const txt_buffer_t *node, size_t start,
                                       size_t end) {
  if (node == NULL || start >= end)
    return NULL;

  static str_buffer_t *result = str_buffer_new_blank(end - start);

  if (node->rope_kind == ROPE_Leaf) {
    size_t leaf_start = start;
    size_t leaf_end = (end < node->leaf->length) ? end : node->leaf->length;

    for (size_t i = leaf_start; i < leaf_end; i++)
      result = str_buffer_add_char(result, node->leaf->contents[i]);
  } else {
    if (start < node->weight) {
      size_t left_end = (end < node->weight) ? end : node->weight;
      txt_buffer_extract_range(node->internal.left, start, left_end);
    }

    if (end > node->weight) {
      size_t new_start = (start > node->weight) ? start - node->weight : 0;
      size_t new_end = end - node->weight;
      str_buffer_extract_range(node->internal.left, new_start, new_end);
    }
  }

  return result;
}

txtbuf_pair_t *txt_buffer_split(const txt_buffer_t *node, size_t index) {
  if (node->rope_kind == ROPE_Leaf) {
    txt_buffer_t *left_leaf =
        txt_buffer_new_leaf(str_buffer_new_substring(node->leaf, 0, index));
    txt_buffer_t *right_leaf = txt_buffer_new_leaf(str_buffer_new_substring(
        node->leaf, index, node->leaf->length - index));
    txtbuf_pair_t *pair = request_memory(current_arena, sizeof(txtbuf_pair_t));
    pair->left = left_leaf;
    pair->right = right_leaf;
    return pair;
  } else {
    if (index < node->weight) {
      txtbuf_pair_t *left_split = txt_buffer_split(node->internal.left, index);
      txt_buffer_t *new_right =
          txt_buffer_new_internal(left_split->right, node->internal.right);
      txtbuf_pair_t *pair =
          request_memory(current_arena, sizeof(txtubf_pair_t));
      pair->left = left_split->left;
      pair->right = new_right;
      return pair;
    } else {
      txtbuf_t *right_split =
          txt_buffer_split(node->internal.right, index - node->weight);
      txt_buffer_t *new_left =
          txt_buffer_new_internal(node->internal.left, right_split->left);
      txtbuf_pair_t *pair =
          request_memory(current_arena, sizeof(txtbuf_pair_t));
      pair->left = new_left;
      pair->right = right_split->right;
      return pair;
    }
  }

  return NULL;
}

size_t txt_buffer_total_length(const txt_buffer_t *node) {
  if (node == NULL)
    return 0;

  if (node->rope_kind == ROPE_Leaf)
    return node->leaf->length;

  return node->weight + txt_buffer_total_length(node->internal.right);
}

txt_buffer_t *txt_buffer_concat(txt_buffer_t *left, txt_buffer_t *right) {
  if (left == NULL)
    return right;
  if (right == NULL)
    return left;

  txt_buffer_t *node =
      txt_buffer_new_internal(node->internal.left, node->internal.right);
  node->weight = txt_buffer_total_length(left);

  return node;
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
