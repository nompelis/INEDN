/******************************************************************************
 This is IN's implementation of an EDN parser. Highly experimental!!!!
 Built by changing the JSON parser. This is based on a "recursive descent"
 parsing model.

 Information was taken from the proposed RFC document:
  [1] https://github.com/edn-format/edn
 Information was taken from this source:
  [2] https://learnxinyminutes.com/docs/edn/

 2023/12/28 IN <nompelis@nobelware.com> - Initial creation
 2024/01/12 IN <nompelis@nobelware.com> - Last modification
 ******************************************************************************/

#ifndef _EDN_H_
#define _EDN_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char    BYTE;


//
// structures and initializers for EDN types
//

enum {
   inEDN_Type_TopLevel = 0,       // a very special type that I invented...
   inEDN_Type_Nil     = 1,        // just the word "nil"
   inEDN_Type_Bool    = 2,        // the word "true" or "false"
   inEDN_Type_Int     = 3,
   inEDN_Type_Real    = 4,
   inEDN_Type_String  = 5,
   inEDN_Type_Set     = 6,
   inEDN_Type_List    = 7,
   inEDN_Type_Vector  = 8,
   inEDN_Type_Pair    = 9,
   inEDN_Type_Map     = 10,
   inEDN_Type_Symbol  = 11,
   inEDN_Type_Keyword = 12,
   inEDN_Type_Undef   = 99
};

const static char *inEDN_Type_strings[] = {
      "EDN toplevel",
      "EDN nil",
      "EDN boolean",
      "EDN integer",
      "EDN real",
      "EDN string",
      "EDN set",
      "EDN list",
      "EDN vector",
      "EDN pair",
      "EDN map",
      "EDN symbol",
      "EDN keyword",
      "EDN undef",
      NULL
};


union inEDN_type_u;    // forward declaration

struct inEDN_nil_s {
   // -- common members
   BYTE type;
   BYTE foo[3]; 
   unsigned int idx;
};

struct inEDN_bool_s {
   // -- common members
   BYTE type;
   BYTE foo[3]; 
   unsigned int idx;

   unsigned char b;
};

#define INEDN_BOOL_INIT( ___ ) { \
                        (___).type = inEDN_Type_Bool; \
                        (___).b = 0xff; \
                       }

struct inEDN_int_s {
   // -- common members
   BYTE type;
   BYTE foo[3]; 
   unsigned int idx;

   long l;
};

#define INEDN_INT_INIT( ___ ) { \
                       (___).type = inEDN_Type_Bool; \
                       (___).l = 0; \
                      }

struct inEDN_real_s {
   // -- common members
   BYTE type;
   BYTE foo[3]; 
   unsigned int idx;

   double d;
};

#define INEDN_REAL_INIT( ___ ) { \
                        (___).type = inEDN_Type_Bool; \
                        (___).d = 0.0; \
                       }

struct inEDN_string_s {
   // -- common members
   BYTE type;
   BYTE foo[3];
   unsigned int idx;

   size_t size;
   char* string;
};

#define INEDN_STRING_INIT( ___ ) { \
                          (___).type = inEDN_Type_String; \
                          (___).size = 0; \
                          (___).string = NULL; \
                         }

struct inEDN_pair_s {
   // -- common members
   BYTE type;
   BYTE foo[3];
   unsigned int idx;

   union inEDN_type_u *key;
   union inEDN_type_u *value;
   struct inEDN_pair_s *prev,*next;    // for pairs in a map
};

#define INEDN_PAIR_INIT( ___ ) { \
                        (___).type = inEDN_Type_Pair; \
                        (___).key = NULL; \
                        (___).value = NULL; \
                        (___).prev = NULL; (___).next = NULL; \
                       }

struct inEDN_map_s {
   // -- common members
   BYTE type;
   BYTE foo[3];
   unsigned int idx;

   size_t num;                 // a count
   struct inEDN_pair_s *data;
   struct inEDN_pair_s *head,*tail;
};

#define INEDN_MAP_INIT( ___ ) { \
                       (___).type = inEDN_Type_Map; \
                       (___).data = NULL; \
                       (___).head = NULL; (___).tail = NULL; \
                      }

struct inEDN_list_s {
   // -- common members
   BYTE type;
   BYTE foo[3];
   unsigned int idx;

   size_t num;                 // a count
   union inEDN_type_u *data;
   // Lists are "sequentially accessed" according to [2]. but we can have
   // random access because of using an array.
};

#define INEDN_LIST_INIT( ___ ) { \
                        (___).type = inEDN_Type_List; \
                        (___).num = 0; \
                        (___).data = NULL; \
                       }

struct inEDN_vector_s {
   // -- common members
   BYTE type;
   BYTE foo[3];
   unsigned int idx;

   size_t num;                 // a count
   union inEDN_type_u *data;
   // Vectors are "random access" acording to [2], and thus an array is needed.
};

#define INEDN_VECTOR_INIT( ___ ) { \
                          (___).type = inEDN_Type_Vector; \
                          (___).num = 0; \
                          (___).data = NULL; \
                         }

struct inEDN_set_s {
   // -- common members
   BYTE type;
   BYTE foo[3]; 
   unsigned int idx;

   size_t num;                 // a count
   union inEDN_type_u *data;
   // Sets are unordered, but we will use an array anyway.
};

#define INEDN_SET_INIT( ___ ) { \
                       (___).type = inEDN_Type_Set; \
                       (___).num = 0; \
                       (___).data = NULL; \
                      }

struct inEDN_symbol_s {
   // -- common members
   BYTE type;
   BYTE foo[3];
   unsigned int idx;

   size_t size;
   char* string;
};

#define INEDN_SYMBOL_INIT( ___ ) { \
                          (___).type = inEDN_Type_String; \
                          (___).size = 0; \
                          (___).string = NULL; \
                         }
struct inEDN_keyword_s {
   // -- common members
   BYTE type;
   BYTE foo[3];
   unsigned int idx;

   size_t size;
   char* string;
};

#define INEDN_KEYWORD_INIT( ___ ) { \
                          (___).type = inEDN_Type_Keyword; \
                          (___).size = 0; \
                          (___).string = NULL; \
                         }


struct inEDN_toplevel_s {
   // -- common members
   BYTE type;
   BYTE foo[3]; 
   unsigned int idx;
   
   size_t num;                 // a count
   union inEDN_type_u *array;
};

#define INEDN_TOPLEVEL_INIT( ___ ) { \
                            (___).type = inEDN_Type_TopLevel; \
                            (___).num = 0; \
                            (___).array = NULL; \
                           }

// the generic type object
union inEDN_type_u {
   struct inEDN_toplevel_s top;     // a very special case
   struct inEDN_nil_s nil;
   struct inEDN_pair_s pair;
   struct inEDN_map_s map;
   struct inEDN_list_s list;
   struct inEDN_vector_s vector;
   struct inEDN_bool_s boolean;
   struct inEDN_int_s integer;
   struct inEDN_real_s real;
   struct inEDN_symbol_s symbol;
   struct inEDN_keyword_s keyword;
};

#define INEDN_TYPE_INIT( ___ ) { \
                        (___).top.type = inEDN_Type_Undef; \
                        (___).top.idx = 0; \
                       }

//
// a struct for maintaining a parsing state object
//

struct inEDN_state_s {
   FILE* fp;
   const char* ptr;
   char msg[64];
};

// Function prototypes

void print_type_info( void );

BYTE inEDN_Convert_ASCIIbyte( const char* string );

void inEDN_Show_ByteBits( FILE *fp, BYTE b );

int inEDN_Parse( struct inEDN_state_s* es,
                 union inEDN_type_u* edn );

int inEDN_UnrollToplevel( struct inEDN_state_s* es,
                          struct inEDN_toplevel_s *top );


#ifdef __cplusplus
}
#endif

#endif
