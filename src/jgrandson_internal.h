// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#pragma once

#include "jgrandson.h"

// All public API functions return an error if that function belongs to a
// jg_state that is incompatible with the current state.
enum jg_state {
    JG_STATE_INIT = 0, // Can only transition to PARSE or SET
    JG_STATE_PARSE = 1, // Can only transition to GET or INIT
    JG_STATE_GET = 2, // Can only transition to INIT
    JG_STATE_SET = 3, // Can only transition to GENERATE or INIT
    JG_STATE_GENERATE = 4 // Can only transition to INIT
};

////////////////////////////////////////////////////////////////////////////////
// struct definitions for JG_STATE_PARSE and JG_STATE_GET //////////////////////

// The .json member in jg_val_in, jg_arr, and jg_obj structs is a pointer to the
// location of the corresponding JSON value within the parsed JSON text
// (jg->json_text). Aside from functioning as a (not null-terminated) string
// representation of JG_TYPE_STR and JG_TYPE_NUM, .json also allows
// jg_get_error_str() to prepend/append JSON text context for errors returned
// from any of the getter functions.

struct jg_val_in {
    union {
        struct jg_arr * arr; // If .type is ARR
        struct jg_obj * obj; // If .type is OBJ
        char const * json; // If .type is not ARR nor OBJ -- NOT null-terminated
    };
    union {
        uint32_t byte_c; // If .type is NUM or STR
        bool bool_is_true; // If .type is BOOL
    };
    enum jg_type type; // The JSON type this value belongs to (see jgrandson.h)
};

struct jg_arr {
    char const * json; // Same meaning as .json in jg_val_in has for non-arr/obj
    size_t elem_c;
    struct jg_val_in elems[]; // flexible array member
};

struct jg_pair {
    struct jg_val_in key; // .type must be STR
    struct jg_val_in val;
};

struct jg_obj {
    char const * json; // Same meaning as .json in jg_val_in has for non-arr/obj
    size_t pair_c;
    struct jg_pair pairs[]; // flexible array member
};

////////////////////////////////////////////////////////////////////////////////
// struct definitions for JG_STATE_SET and JG_STATE_GENERATE ///////////////////

struct jg_val_out {
    union {
        struct jg_arr_node * arr; // If .type is ARR
        struct jg_obj_node * obj; // If .type is OBJ
        char const * str; // If .type is STR or NUM -- null-terminated
    };
    union {
        bool bool_is_true; // If .type is BOOL
        bool str_needs_free; // True if set with jg_set_..._astr() (with an 'a')
    };
    enum jg_type type; // The JSON type this value belongs to (see jgrandson.h)
};

// Unlike the jg_arr and jg_obj structs for parsing and getting, arrays and
// objects for setting and generating are implemented as linked lists, because
// they allow a more flexible setter API (by not forcing the setter caller to
// declare array and object lengths in advance of setting their elements).

struct jg_arr_node {
    struct jg_val_out elem;
    struct jg_arr_node * next;
};

struct jg_obj_node {
    struct jg_val_out val;
    char * key; // null-terminated
    struct jg_obj_node * next;
};

////////////////////////////////////////////////////////////////////////////////
// Main jgrandson (jg_t) struct definition /////////////////////////////////////

union jg_err_val {
    intmax_t i; // getter min/max int boundary errors
    uintmax_t u; // getter min/max uint boundary errors
    size_t s; // getter min_*_c/max_*_c element count boundary errors
    int errn; // errno set through external function calls
};

struct jgrandson {
    union {
        struct jg_val_in root_in; // Root JSON value if .state is PARSE or GET
        struct jg_val_out root_out; // Root JSON value if .state SET or GENERATE
    };    
    // Members below other than .ret and .state are for parsing/getting only.
    char const * json_text; // The start of the JSON text string to be parsed
    char const * json_cur; // Current parsing position within .json_text
    char const * json_over; // The byte following the end of the .json_text buf
    
    char const * err_str; // Ref to the string returned by jg_get_err_str()
    char * custom_err_str; // A copy of a "..._reason" getter option
    union jg_err_val err_val; // Val associated with the last .ret err condition
    
    bool json_text_needs_free; // True if jg_parse_astr() was called
    bool err_str_needs_free; // True for all non-static error strings

    jg_ret ret; // The last jg_ret value returned by a public API function
    enum jg_state state;
};

////////////////////////////////////////////////////////////////////////////////
// jg_util.c prototypes (internal) /////////////////////////////////////////////

void free_json_text( // Only free()s if jg->json_text_needs_free
    jg_t * jg
);

void free_err_str( // Only free()s if jg->err_str_needs_free
    jg_t * jg
);

jg_ret set_custom_err_str(
    jg_t * jg,
    char const * custom_err_str
);

bool is_utf8_continuation_byte(
    char byte
);
