#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#define EPSILON_TRANS -1

extern Arena *current_arena;

typedef struct NFATrans nfa_trans_t;
typedef struct NFAState nfa_state_t;
typedef struct NFAMain nfa_main_t;
typedef struct RETree regex_tree_t;

struct NFATrans {
  char32_t symbol;
  nfa_state_t *target;
  nfa_trans_t *next;
  nfa_trans_t *prev;
};

struct NFAState {
  int id;
  bool is_accepting;
  nfa_trans_t *trans;
  nfa_trans_t *eps_trans;
  nfa_state_t *next;
  nfa_state_t *prev;
};

struct NFAMain {
  nfa_state_t *start_state;
  nfa_state_t *states;
  const char32_t *regexp;
};

struct RETree {
  enum REKind {
    RE_Concat,
    RE_Union,
    RE_Closure,
    RE_CharRange,
    RE_Bracket,
    RE_EscapedChar,
    RE_Char,
    RE_CharCluster,
    RE_Backtrack,
    RE_InitLeaf,
  } re_kind;

  union {
    str_buffer_t *v_buffer;

    struct {
      char32_t start;
      char32_t end;
    } v_range;

    char32_t v_char;
    size_t v_backtrack;
  };

  struct RETree *children;
  struct RETree *next;
  struct RETree *prev;
};

static int state_id_counter = 0;

nfa_state_t *nfa_state_new(bool is_accepting) {
  nfa_state_t *state = request_memory(current_arena, sizeof(nfa_state_t));
  state->id = state_id_counter++;
  state->is_accepting = is_accepting;
  state->trans = NULL;
  state->next = NULL;
  state->prev = NULL;
}

bool nfa_state_list_empty(nfa_state_t **list) {
  if (list == NULL || *list == NULL)
    return true;
  return false;
}

nfa_state_t *nfa_state_list_append(nfa_state_t **list, nfa_state_t *state) {
  if (nfa_state_list_empty(list)) {
    *list = state;
    return *list;
  }

  nfa_state_t *head = *list;

  while (head->next != NULL)
    head = head->next;

  state->prev = head;
  head->next = state;

  return state;
}

nfa_state_t *nfa_state_list_copy(nfa_state_t **list) {
  if (nfa_state_list_empty(list))
    return NULL;

  nfa_state_t *head = *list;
  nfa_state_t *copy_list =
      duplicate_memory(current_arena, head, sizeof(nfa_state_t));

  head = head->next;
  while (head != NULL) {
    nfa_state_t *copy_item =
        duplicate_memory(current_arena, head, sizeof(nfa_state_t));
    nfa_state_list_append(&copy_list, copy_item);
    head = head->next;
  }

  return copy_list;
}

nfa_state_t *nfa_state_list_pop(nfa_state_t **list) {
  if (nfa_state_list_empty(list))
    return NULL;

  nfa_state_t *head = *list;

  while (head->next != NULL)
    head = head->next;

  if (head->prev != NULL)
    head->prev->next = NULL;
  else
    *list = NULL;

  head->prev = NULL;

  return head;
}

bool nfa_state_list_contains(nfa_state_t **list, nfa_state_t state) {
  if (nfa_state_list_empty(list))
    return false;

  nfa_state_t *head = *list;

  while (head != NULL) {
    if (head->id == state->id)
      return true;
    head = head->next;
  }

  return false;
}

nfa_trans_t *nfa_trans_new(char32_t symbol, nfa_state_t *target) {
  nfa_trans_t *trans = request_memory(current_arena, sizeof(nfa_trans_t));
  trans->symbol = symbol;
  trans->target = target;
  trans->next = NULL;
  trans->prev = NULL;
}

bool nfa_trans_list_empty(nfa_trans_t **list) {
  if (list == NULL || *list == NULL)
    return true;
  return false;
}

nfa_trans_t *nfa_trans_list_append(nfa_trans_t **list, nfa_trans_t *trans) {
  if (nfa_trans_list_empty(list)) {
    *list = trans;
    return *list;
  }

  nfa_trans_t *head = *list;

  while (head->next != NULL)
    head = head->next;

  trans->prev = head;
  head->next = trans;

  return trans;
}

nfa_trans_t *nfa_trans_list_filter_eps_trans(nfa_trans_t **list) {
  if (nfa_trans_list_empty(list))
    return NULL;

  nfa_trans_t *filt_list = NULL;
  nfa_trans_t *head = *list;

  while (head != NULL) {
    if (head->symbol == EPSILON_TRANS)
      nfa_trans_list_append(&filt_list, head);
    head = head->next;
  }

  return filt_list;
}

nfa_state_t *epsilon_closure(nfa_state_t **state_list) {
  nfa_state_t *closure = nfa_state_list_copy(state_list);
  nfa_state_t *stack = nfa_state_list_copy(state_list);

  while (!nfa_state_list_empty(&stack)) {
    nfa_state_t *top = nfa_state_list_pop(&stack);
    nfa_trans_t *eps_trans = top->eps_trans;

    while (eps_trans != NULL) {
      if (!nfa_state_list_contains(&closure, eps_trans->target)) {
        nfa_state_list_append(&closure, eps_trans->target);
        nfa_state_list_append(&stack, eps_trans->target);
      }

      eps_trans = eps_trans->next;
    }
  }

  return closure;
}

bool nfa_simulate_and_match(nfa_main_t *nfa, const char32_t *input,
                            size_t input_length) {
  nfa_state_t *current_states = epsilon_closure(nfa->start_state);

  for (size_t i = 0; i < input_length; i++) {
    nfa_state_t *next_states = NULL;
    nfa_state_t *head_current = current_states;

    while (head_current != NULL) {
      nfa_trans_t *head_trans = head_current->trans;

      while (head_trans != NULL) {
        if (head_trans->symbol == input[i])
          nfa_state_list_append(&next_states, head_trans->target);

        head_trans = head_trans->next;
      }
    }

    head_current = head_current->next;
    current_states = epsilon_closure(next_states);

    if (nfa_state_list_empty(current_states))
      return false;
  }

  nfa_state_t *head_current = current_states;

  while (head_current != NULL) {
    if (head_current->is_accepting)
      return true;

    return false;
  }
}

regex_tree_t *regex_tree_new_blank(enum REKind kind) {
  regex_tree_t *tree = request_memory(current_arena, sizeof(regex_tree_t));

  tree->re_kind = kind;
  tree->children = NULL;
  tree->next = NULL;
  tree->prev = NULL;

  return tree;
}

regex_tree_t *regex_tree_list_append(regex_tree_t **list, regex_tree_t *leaf) {
  if (list == NULL || *list == NULL) {
    *list = leaf;
    return *list;
  }

  regex_tree_t *head = *list;

  while (head->next != NULL)
    head = head->next;

  leaf->prev = head;
  head->next = leaf;

  return head;
}

nfa_t *construct_nfa_from_regexp(const regex_tree_t *re_tree) {}
