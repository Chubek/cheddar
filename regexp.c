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
  struct NFATrans *next;
  struct NFATrans *prev;
};

struct NFAState {
  int id;
  bool is_accepting;
  nfa_trans_t *trans;
  nfa_trans_t *eps_trans;
  struct NFAState *next;
  struct NFAState *prev;
};

struct NFAMain {
  nfa_state_t *start_state;
  nfa_state_t *accept_state;
  nfa_state_t *states;
  struct NFAMain *next;
  struct NFAMain *prev;
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

nfa_state_t *nfa_state_add_transition(nfa_state_t *state, char32_t symbol,
                                      nfa_state_t *target) {
  nfa_trans_list_append(&state->trans, nfa_trans_new(symbol, target));
}

nfa_state_t *nfa_state_add_eps_transition(nfa_state_t *state,
                                          nfa_state_t *target) {
  nfa_trans_list_append(&state->eps_trans,
                        nfa_trans_new(EPSILON_TRANS, target));
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

nfa_main_t *nfa_main_new(nfa_state_t *start_state, nfa_state_t *accept_state) {
  nfa_main_t *nfa = request_memory(current_arena, sizeof(nfa_main_t));
  nfa->start_state = start_state;
  nfa->accept_state = accept_state;
  nfa->states = NULL;
  nfa->next = NULL;
  nfa->prev = NULL;
  nfa_state_list_append(&nfa->states, start_state);
  nfa_state_list_append(&nfa->states, end_state);
  return nfa;
}

nfa_main_t *nfa_main_new_literal(char32_t symbol) {
  nfa_state_t *start_state = nfa_state_new(false);
  nfa_state_t *accept_state = nfa_state_new(true);
  nfa_state_add_transiton(start_state, symbol, accept_state);
  nfa_main_t *nfa = nfa_main_new(start_state, accept_state);
  return nfa;
}

nfa_main_t *nfa_main_new_union(nfa_main_t *nfa_a, nfa_main_t *nfa_b) {
  nfa_state_t *new_start_state = nfa_state_new(false);
  nfa_state_t *new_accept_state = nfa_state_new(true);

  nfa_state_add_eps_transition(new_start_state, nfa_a->start_state);
  nfa_state_add_eps_transition(new_start_state, nfa_b->start_state);

  nfa_state_add_eps_transition(nfa_a->accept_state, new_accept_state);
  nfa_state_add_eps_transition(nfa_b->accept_state, new_accept_state);

  nfa_main_t *nfa = nfa_main_new(new_start_state, new_accept_state);
  return nfa;
}

nfa_main_t *nfa_main_new_closure(nfa_main_t *nfa) {
  nfa_state_t *new_start_state = nfa_state_new(false);
  nfa_state_t *new_accept_state = nfa_state_new(true);

  nfa_state_add_eps_transition(new_start_state, new_accept_state);
  nfa_state_add_eps_transition(nfa->accept_state, nfa->start_state);
  nfa_state_add_eps_transition(new_start_state, nfa->start_state);
  nfa_state_add_eps_transition(nfa->accept_state, new_accept_state);

  nfa_main_t *nfa = nfa_main_new(new_start_state, new_accept_state);
  return nfa;
}

nfa_main_t *nfa_main_new_concat(nfa_main_t *nfa_a, nfa_main_t *nfa_b) {
  nfa_state_add_eps_transition(nfa_a->accept_state, nfa_b->start_state);

  nfa_main_t *nfa = nfa_main_new(nfa_a->start_state, nfa_b->accept_state);
  return nfa;
}

nfa_main_t *nfa_main_list_append(nfa_main_t **list, nfa_main_t *nfa) {
  if (list == NULL || *list == NULL) {
    *list = nfa;
    return *list;
  }

  nfa_main_t *head = *list;

  while (head->next != NULL)
    head = head->next;

  nfa->prev = head;
  head->next = nfa;

  return head;
}

nfa_main_t *nfa_main_list_pop(nfa_main_t **list) {
  if (list == NULL || *list == NULL) {
    nfa_main_t *tail = *list;
    *list = NULL;
    return tail;
  }

  nfa_main_t *head = *list;

  while (head->next != NULL)
    head = head->next;

  if (head->prev != NULL)
    head->prev->next = NULL;

  head->prev = NULL;

  return head;
}

str_buffer_t *add_concat_operator_to_regex(str_buffer_t *regex) {
  str_buffer_t *result =
      duplicate_memory(current_arena, regex, sizeof(str_buffer_t));

  for (size_t i = 0; i < result->length - 1; i++) {
    char32_t curr = result->contents[i];
    char32_t peek = result->contents[i + 1];
    if (isalnum(curr) && isalnum(peek))
      result = str_buffer_splice_char(result, i, ++i, U'\0');
    else if ((isalnum(curr) || curr == U'*' || curr == U')') && peek == U'(')
      result = str_buffer_splice_char(result, i, ++i, U'\0');
    else if ((curr == U'*' || curr == U')') && isalnum(peek))
      result = str_buffer_splice_char(result, i, ++i, U'\0');
    else
      continue;
  }

  return result;
}

str_buffer_t *get_regex_to_postfix(const str_buffer_t *regex) {
  str_buffer_t *result = str_buffer_new_blank(regex->length);
  char32_t const *operator_stack = calloc(1, regex->length);
  ssize_t stack_pointer = -1;

  for (size_t i = 0; i < regex->length; i++) {
    char32_t curr = regex->contents[i];
    if (isalnum(curr))
      result = str_buffer_add_char(result, curr);
    else if (curr == U'(')
      operator_stack[++stack_pointer] = curr;
    else if (curr == U')') {
      while (stack_pointer != 0 && operator_stack[stack_pointer] != U'(')
        result = str_buffer_add_char(result, operator_stack[stack_pointer--]);

      if (stack_pointer != 0 && operator_stack[stack_pointer] == U'(')
        stack_pointer--;
    } else if (curr == U'*' || curr == U'\0' || curr == U'|') {
      while (stack_pointer != 0 &&
             get_regex_operator_precedence(operator_stack[stack_pointer]) >=
                 get_regex_operator_precedence(cur))
        result = str_buffer_add_char(result, operator_stack[stack_pointer--]);
      operator_stack[++stack_pointer] = curr;
    }
  }

  return result;
}

int get_regex_operator_precedence(char32_t operator) {
  switch (operator) {
  case U'*':
    return 3;
  case U'\0':
    return 2;
  case U'|':
    return 1;
  default:
    return 0;
  }
}

nfa_main_t *nfa_main_from_regexp(const str_buffer_t *regexp) {
  nfa_main_t *nfa_stack = NULL;

  for (size_t i = 0; i < regexp->size; i++) {
    switch (regexp->contents[i]) {
    case U'|':
      nfa_main_t *nfa_b = nfa_main_list_pop(&nfa_stack);
      nfa_main_t *nfa_a = nfa_main_list_pop(&nfa_stack);
      nfa_main_t union_nfa = nfa_main_new_union(nfa_a, nfa_b);
      nfa_main_list_append(&nfa_stack, union_nfa);
      continue;
    case U'*':
      nfa_main_t *nfa = nfa_main_list_pop(&nfa_stack);
      nfa_main_t *closure_nfa = nfa_main_new_closure(nfa);
      nfa_main_list_append(&nfa_stack, closure_nfa);
      continue;
    case U'\0':
      nfa_main_t *nfa_b = nfa_main_list_pop(&nfa_stack);
      nfa_main_t *nfa_a = nfa_main_list_pop(&nfa_stack);
      nfa_main_t concat_nfa = nfa_main_new_concat(nfa_a, nfa_b);
      nfa_main_list_append(&nfa_stack, concat_nfa);
      continue;
    default:
      nfa_main_t *literal_nfa = nfa_main_new_literal(regexp->contents[i]);
      nfa_main_list_append(&nfa_stack, literal_nfa);
      continue;
    }
  }

  return nfa_main_list_pop(&nfa_stack);
}
