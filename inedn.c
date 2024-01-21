/******************************************************************************
 This is IN's implementation of an EDN parser. Highly experimental!!!!
 Built by changing the JSON parser. This is based on a "recursive descent"
 parsing model.

 Information was taken from the proposed RFC document:
  [1] https://github.com/edn-format/edn
 Information was taken from this source:
  [2] https://learnxinyminutes.com/docs/edn/
 The Wikipedia of the ASCII set was useful:
  [3] https://en.wikipedia.org/wiki/ASCII

 2023/12/28 IN <nompelis@nobelware.com> - Initial creation
 2024/01/12 IN <nompelis@nobelware.com> - Last modification
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _DEBUG_
#include <unistd.h>
#endif

#include "inedn.h"


// -------------- helper functions --------------
//
// Strings of built-in documentation
//

const char *inEDN_strings[] = {
 "The word \"null\" by itself",
   "[ This is it. Nothing else to know... ]",

 "The (special) TopLevel type",
   "[ This is a type that is meant to work as a top-level root node for ]"
   "[ parsing multiple types that can appear concatanated. ]",

 "The Object (a \"set\" of key-value pairs) type",
   "[ This is a _set_ of key-value pairs. The whole EDN block itself is ]"
   "[ an \"object\". A \"key-value\" pair is customary of a map construct. ]",

 "The String type",
   "[ This an string object, which is in Unicode format. ]"
   "[ The characters are contained within \"\" as usual. ]",

 "The Array type",
   "[ This an \"ordered list\" object. ]"
   "[ The list's items are typically contained within square brackets. ]",

 "The Number type",
   "[ This is a signed number, without distrinction between a real number ]"
   "[ and an integer, and it may use exponential \"E\" notation. ]",

 "The Boolean type",
   "[ This two-state object must be one of the words \"true\" or \"false\" ]"
   "[ in all lower case latters (I think). ]",

 "The (special) Pair type",
   "[ This is NOT an official type in EDN, but I am using it such that I ]"
   "[ can store the key-value pairs of the \"Object\" type. ]",

 NULL
};


//
// Function to print info for the format types
//

void inEDN_ShowTypeInfo( const char *strings[] )
{
   int n;
   const char *s;

   fprintf( stdout, "=====================================================\n" );
   n = 0;
   while( (s = strings[n++]) != NULL ) {
      if( s[0] == '[' ) {
         int k=0;
         const char *s2 = s;
         char iprint=0;
         while( s2[k] != '\0' ) {
            if( s[k] == '[' ) {
               fprintf( stdout, "  " );
               iprint = 1;
               ++k;
            }
            if( iprint == 1 ) {
               fprintf( stdout, "%c", s[k] );
               ++k;
            }
            if( s[k] == ']' ) {
               fprintf( stdout, "\n" );
               iprint = 0; 
               ++k;
            }
         }
      } else {
         fprintf( stdout, " * %s\n", s );
      }
   }

   fprintf( stdout, "=====================================================\n" );
}


void print_type_info( void )
{
#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG]  Short name types \n" );
   inEDN_ShowTypeInfo( inEDN_Type_strings );
   fprintf( stdout, " [DEBUG]  Long descriptions of types \n" );
#endif
   inEDN_ShowTypeInfo( inEDN_strings );
}


//
// Function to convert from an ASCII "0xXX" representation to the
// "unsigned char" type known as "BYTE"
// (Uses a naive string parsing approach on purpose to be instructive.)
// (I really need to change this...)
//

BYTE inEDN_Convert_ASCIIbyte( const char* string )
{
   BYTE b = 0;
   unsigned int ip;
   BYTE m,l;
   char str[2];

   sprintf( str, "%c", string[0] );
   sscanf( str, "%x", &ip );
   m = (BYTE) ip;
   m = m << 4;

   sprintf( str, "%c", string[1] );
   sscanf( str, "%x", &ip );
   l = (BYTE) ip;
   b = m + l;

   return b;
}


//
// Function to print the 8 bits of a "BYTE" type
//

void inEDN_Show_ByteBits( FILE *fp, BYTE b )
{
   BYTE mask = 0x01;       // (initial) 8-bit mask "0000 0001"
   int n=0;

   fprintf( fp, "Byte: 0x%.2x   ", b );
   fprintf( fp, "Bits: " );
   for(n=7;n>=0;--n) {
      fprintf( fp, "%d", (b >> n) & mask );
   }
   fprintf( fp, "\n" );
}


//
// Function to display a generic non-container object
//

int inEDN_UnrollList( struct inEDN_state_s* es,   // forward declaration
                      struct inEDN_list_s *list, int depth );
int inEDN_UnrollMap( struct inEDN_state_s* es,
                     struct inEDN_map_s *map, int depth );

int inEDN_DisplayGeneric( struct inEDN_state_s* es,
                          union inEDN_type_u* u, int depth )
{
   if( es == NULL || u == NULL ) return 1;

   FILE *fp = es->fp;

   if( u->top.type == inEDN_Type_Undef) {
      fprintf( fp, " [Error]  Item is of \"Undefined\" type \n" );
   } else {
#ifdef _DEBUG_UNROLL_
      fprintf( fp, " [INFO]  Displaying \"%s\" (%d) at %p \n",
               inEDN_Type_strings[ u->top.type ], u->top.type, u );
#endif

      switch( u->top.type ) {
       case inEDN_Type_Nil:
         for(int n=0;n<depth;++n) printf( " " );
         fprintf( fp, " nil \n" );
       break;
       case inEDN_Type_Bool:
         for(int n=0;n<depth;++n) printf( " " );
       { struct inEDN_bool_s* p = (struct inEDN_bool_s*) u;
         if( p->b ) fprintf( fp, " true \n" );
         else       fprintf( fp, " false \n" );
       }
       break;
       case inEDN_Type_Int:
         for(int n=0;n<depth;++n) printf( " " );
         fprintf( fp, " %ld \n", ((struct inEDN_int_s*) u)->l );
       break;
       case inEDN_Type_Real:
         for(int n=0;n<depth;++n) printf( " " );
         fprintf( fp, " %19.12e \n", ((struct inEDN_real_s*) u)->d );
       break;
       case inEDN_Type_String:
         for(int n=0;n<depth;++n) printf( " " );
         fprintf( fp, " \"%s\" \n", ((struct inEDN_string_s*) u)->string );
       break;
       case inEDN_Type_Set:
         inEDN_UnrollList( es, (struct inEDN_list_s*) u, depth+1 );
       break;
       case inEDN_Type_List:
         inEDN_UnrollList( es, (struct inEDN_list_s*) u, depth+1 );
       break;
       case inEDN_Type_Vector:
         inEDN_UnrollList( es, (struct inEDN_list_s*) u, depth+1 );
       break;
       case inEDN_Type_Map:
         inEDN_UnrollMap( es, (struct inEDN_map_s*) u, depth+1 );
       break;
       case inEDN_Type_Symbol:
         for(int n=0;n<depth;++n) printf( " " );
         fprintf( fp, " %s \n", ((struct inEDN_symbol_s*) u)->string );
       break;
       case inEDN_Type_Keyword:
         for(int n=0;n<depth;++n) printf( " " );
         fprintf( fp, " %s \n", ((struct inEDN_keyword_s*) u)->string );
       break;
       default:
         fprintf( fp, " This is in error! Unspecified type! \n" );
      }
   }

   return 0;
}


//
// Function to unroll/list the contents a "Map" type (for diagnostics)
//

int inEDN_UnrollMap( struct inEDN_state_s* es,
                     struct inEDN_map_s *map, int depth )
{
   if( es == NULL || map == NULL ) return 1;

   FILE *fp = es->fp;
#ifdef _DEBUG_UNROLL_
   fprintf( fp, " [INFO]  Unrolling map at %p of size %ld \n",
            map, map->num );
#endif

   for(int n=0;n<depth;++n) printf( " " );
   printf( " { \n" );

   for(size_t n=0;n<map->num;++n) {
      struct inEDN_pair_s *p = &( map->data[n] );
      inEDN_DisplayGeneric( es, p->key, depth+1 );
      inEDN_DisplayGeneric( es, p->value, depth+1 );
   }

   for(int n=0;n<depth;++n) printf( " " );
   printf( " } \n" );

   return 0;
}


//
// Function to unroll/list the contents a the "List/Vector/Set" objects
//

int inEDN_UnrollList( struct inEDN_state_s* es,
                      struct inEDN_list_s *list, int depth )
{
   if( es == NULL || list == NULL ) return 1;

   FILE *fp = es->fp;

   if( list->type == inEDN_Type_List ) {
#ifdef _DEBUG_UNROLL_
      fprintf( fp, " [INFO]  Unrolling \"List\" at %p of size %ld \n",
               list, list->num );
#endif
      for(int n=0;n<depth;++n) printf( " " );
      fprintf( fp, " ( \n" );
   } else if( list->type == inEDN_Type_Vector ) {
#ifdef _DEBUG_UNROLL_
      fprintf( fp, " [INFO]  Unrolling \"Vector\" at %p of size %ld \n",
               list, list->num );
#endif
      for(int n=0;n<depth;++n) printf( " " );
      fprintf( fp, " [ \n" );
   } else if( list->type == inEDN_Type_Set ) {
#ifdef _DEBUG_UNROLL_
      fprintf( fp, " [INFO]  Unrolling \"Set\" at %p of size %ld \n",
               list, list->num );
#endif
      for(int n=0;n<depth;++n) printf( " " );
      fprintf( fp, " #{ \n" );
   } else {
      fprintf( fp, " [Error]  Item is not List/Vector/Set \n" );
      return 1;
   }

   for(size_t n=0;n<list->num;++n) {
      union inEDN_type_u *u = &( list->data[n] );

      if( u->top.type == inEDN_Type_Undef) {
         fprintf( fp, " [Error]  Item is of \"Undefined\" type \n" );
      } else {
      // fprintf( fp, " [INFO]  Displaying \"%s\" (%d) at %p \n",
      //          inEDN_Type_strings[ u->top.type ], u->top.type, u );

         switch( u->top.type ) {
          case inEDN_Type_Nil:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          case inEDN_Type_Bool:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          case inEDN_Type_Int:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          case inEDN_Type_Real:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          case inEDN_Type_String:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          case inEDN_Type_Set:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          case inEDN_Type_List:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          case inEDN_Type_Vector:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          case inEDN_Type_Map:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          case inEDN_Type_Symbol:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          case inEDN_Type_Keyword:
            inEDN_DisplayGeneric( es, u, depth+1 );
          break;
          default:
            fprintf( fp, " This is in error! Unspecified type! \n" );
         }
      }
   }

   for(int n=0;n<depth;++n) printf( " " );
   if( list->type == inEDN_Type_List ) {
      fprintf( fp, " ) \n" );
   } else if( list->type == inEDN_Type_Vector ) {
      fprintf( fp, " ] \n" );
   } else if( list->type == inEDN_Type_Set ) {
      fprintf( fp, " } \n" );
   }

   return 0;
}


//
// Function to unroll/list the contents a the "toplevel" object
//

int inEDN_UnrollToplevel( struct inEDN_state_s* es,
                          struct inEDN_toplevel_s *top )
{
   if( es == NULL || top == NULL ) return 1;

   FILE *fp = es->fp;
   fprintf( fp, " [INFO]  Unrolling \"Toplevel\" at %p of size %ld \n",
            top, top->num );

   for(size_t n=0;n<top->num;++n) {
      union inEDN_type_u* u = (union inEDN_type_u*) &( top->array[n] );
      inEDN_DisplayGeneric( es, u, 0 );
   }

   return 0;
}


// -------------- parsing functions --------------


int inEDN_ClassifySpecial( struct inEDN_state_s* es )
{
   if( es == NULL ) {
      fprintf( stdout, "Error: null EDN pointer \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Classifying __Special__ \n" );
#endif

   int ierr=0;
   const char* ptr = es->ptr;
   unsigned char ic=0xff,n=0;
#ifdef _DEBUG_SPECIAL2_
   if(fp) fprintf( fp, " [DEBUG]  Starting pointer at -->%c<-- \n", ptr[0] );
#endif
   while( ierr == 0 ) {
#ifdef _DEBUG_SPECIAL3_
      if(fp) fprintf( fp, " [DEBUG]  Character: \"%c\" \n", ptr[n] );
#endif
      if( ic == 0xff ) {
         if( ptr[n] == 'n' || ptr[n] == 'N' ) ic=2;
         if( ptr[n] == 'f' || ptr[n] == 'F' ) ic=1;
         if( ptr[n] == 't' || ptr[n] == 't' ) ic=0;
         if( ic == -1 ) ierr=101;    // unexpected character
      } else if( ic == 0 ) {
         switch( n ) {
          case 1:
            if( ptr[n] != 'r' && ptr[n] != 'R' ) ierr = 101;
          break;
          case 2:
            if( ptr[n] != 'u' && ptr[n] != 'U' ) ierr = 101;
          break;
          case 3:
            if( ptr[n] != 'e' && ptr[n] != 'E' ) ierr = 101; else ierr=100;
          break;
          default:
            ierr=101;
         }
      } else if( ic == 1 ) {
         switch( n ) {
          case 1:
            if( ptr[n] != 'a' && ptr[n] != 'A' ) ierr = 101;
          break;
          case 2:
            if( ptr[n] != 'l' && ptr[n] != 'L' ) ierr = 101;
          break;
          case 3:
            if( ptr[n] != 's' && ptr[n] != 'S' ) ierr = 101;
          break;
          case 4:
            if( ptr[n] != 'e' && ptr[n] != 'E' ) ierr = 101; else ierr=100;
          break;
          default:
            ierr=101;
         }
      } else if( ic == 2 ) {
         switch( n ) {
          case 1:
            if( ptr[n] != 'i' && ptr[n] != 'I' ) ierr = 101;
          break;
          case 2:
            if( ptr[n] != 'l' && ptr[n] != 'L' ) ierr = 101; else ierr=100;
          break;
          default:
            ierr=101;
         }
      }
      // over-write errors in case of truncation
      if( ptr[0] == '\0' ) ierr=102;   // detect truncated file

      ++n;
   }

   if( ierr == 100 ) {
      if( ic == 0 ) ierr=5001;
      else if( ic == 1 ) ierr=5002;
      else if( ic == 2 ) ierr=5003;
   }

   if( ierr == 101 ) {
      if(fp) fprintf( fp, " [Error]  Token is garbled \n" );
      ierr=9999;
      ic=0xff;
   }

   if( ierr == 102 ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG]  Detected truncated stream \n" );
#endif
      ierr=1000;
      ic=0xff;
   }

#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Classified __Special__ (%d) \n", ic );
#endif
   return ierr;
}


//
// Function to classify a segment based on its initial byte
//

int inEDN_ClassifyToken( struct inEDN_state_s *es, char c )
{
   if( es == NULL ) return 1;

   FILE *fp = es->fp;

   int iret=1;

   if( c == '\0' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected nullchar \n" );
#endif
      iret = 1000;
   } else if( c == ';' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected \"Comment\"token  \n" );
#endif
      iret = 1001;
   } else if( c == ' ' || c == '\t' || c == ',' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected \"Whitespace\" \n" );
#endif
      iret = 1002;
   } else if( c == '\n' || c == '\r' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected newline \n" );
#endif
      iret = 1003;
   } else if( c == '(' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected \"List\" token \n" );
#endif
      iret = 2001;
   } else if( c == ')' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected end of \"List\" token \n" );
#endif
      iret = 2011;
   } else if( c == '[' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected \"Vector\" token \n" );
#endif
      iret = 2002;
   } else if( c == ']' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected end of \"Vector\" token \n" );
#endif
      iret = 2012;
   } else if( c == '{' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected \"Map\" token \n" );
#endif
      iret = 2003;
   } else if( c == '}' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected end of \"Map/Set\" token \n" );
#endif
      iret = 2013;
   } else if( c == '\"' ) {
      // a string is necessarily enclosed in double quotes (good!)
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected \"String\" token \n" );
#endif
      iret = 3001;
   } else if( c == '#' ) {
      // dispatch character
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected \"Set\" token \n" );
#endif
      iret = 4000;
   } else if( c == 't' || c == 'f' || c == 'n') { 
      // potentially a boolean or nil; we must invoke a parser to find out
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Potentially a \"Boolean/Nil\" token \n" );
#endif
      switch( inEDN_ClassifySpecial( es ) ) {
       case 5001:
#ifdef _DEBUG_CLASSIFY3_
         if(fp) fprintf( fp, " [DEBUG3]  Detected \"true\" \n" );
#endif
         iret = 5001;   // true
       break;
       case 5002:
#ifdef _DEBUG_CLASSIFY3_
         if(fp) fprintf( fp, " [DEBUG3]  Detected \"false\" \n" );
#endif
         iret = 5002;   // false
       break;
       case 5003:
#ifdef _DEBUG_CLASSIFY3_
         if(fp) fprintf( fp, " [DEBUG3]  Detected \"nil\" \n" );
#endif
         iret = 5003;   // nil
       break;
       default:
#ifdef _DEBUG_CLASSIFY3_
         if(fp) fprintf( fp, " [DEBUG3]  Detected \"Symbol\" \n" );
#endif
         iret = 6000;      // must be a "symbol"
      }
   } else if( c == '.' || c == '*' || c == '+' || c == '!' || c == '-' ||
              c == '_' || c == '?' || c == '$' || c == '%' || c == '&' ||
              c == '=' || c == '<' || c == '>' ||
              c == '/' ||   // this is special, but can be a symbol in itself
              (( 65 <= c && c <= 90 ) || ( 97 <= c && c <= 122 ))  // ascii
            ) {
      // Expect ". * + ! - _ ? $ % & = < >". If -, + or . are leading, the next
      // character must be non-numeric. (This can overlap signed integers and
      // reals; classification is preliminary and it can be parsed as a symbol
      // with subsequent re-parsing and re-assignment of the type.)
   ///// I have a feeling I should make a 256-byte array to map directlly to
   ///// the outcome instead of using conditionals. This is a subsequent commit!
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected \"Symbol/Numeric\" token \n" );
#endif
      iret = 6000;      // preliminary classifcation as "symbol"
   } else if( c == ':' ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected \"Keyword\" token \n" );
#endif
      iret = 7000;
   } else if( 48 <= c && c <= 57 ) {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Detected \"Numeric\" token \n" );
#endif
      iret = 8000;      // numeric / may be integer or real
   } else {
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG2]  Unclassified token during detection \n" );
#endif
      iret = 9999;
   }

   return iret;
}


//
// Function to classify a segment following the initial dispatch character
// (Presently, this is "set" "discard" or something else that is "special"
//

int inEDN_ClassifyPound( struct inEDN_state_s *es, char c )
{
   if( es == NULL ) return 1;

   FILE *fp = es->fp;

   int iret=4000;

   if( c == '{' ) {
      iret = 4001;
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG]  Detected dispatch to set \n" );
#endif
   } else if( c == '_' ) {
      iret = 4002;
#ifdef _DEBUG_CLASSIFY2_
      if(fp) fprintf( fp, " [DEBUG]  Detected dispatch to discard \n" );
#endif
   } else {
      printf("Unclassified dispatch ancountered while dev.: \"%c\" \n", c );
      exit(1);
   }

   return iret;
}


//
// Function to allocate a standard type
//

int inEDN_Allocate( struct inEDN_state_s *es,
                    union inEDN_type_u** p )
{
   if( es == NULL || p == NULL ) return 1;

   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Allocating generic type \n" );
#endif

   size_t size = sizeof( union inEDN_type_u );
   *p = (union inEDN_type_u*) malloc( size );
   if( *p == NULL ) {
      sprintf( es->msg, "Error: could not allocate item " );
      if(fp) fprintf( fp, "%s\n", es->msg );
      return -1;
   }

   INEDN_TYPE_INIT( *(*p) )

   return 0;
}


//
// Function to extend a "Toplevel" type's reference array
//

int inEDN_Extend( struct inEDN_state_s *es,
                  struct inEDN_toplevel_s *t )
{
   if( es == NULL || t == NULL ) return 1;

   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Extending Toplevel at %p, items: %ld\n",
            t, t->num );
#endif

   if( t->num < 0 ) return 2;

   size_t isize = (t->num + 1) * sizeof( union inEDN_type_u );
   union inEDN_type_u* p = (union inEDN_type_u*) realloc( t->array, isize );
   if( p == NULL ) {
      sprintf( es->msg, "Error: could not reallocate array of toplevel" );
      if(fp) fprintf( fp, "%s\n", es->msg );
      return -1;
   }

   INEDN_TYPE_INIT( p[ t->num ] )

   t->array = p;
   t->num += 1;

   return 0;
}


//
// Function to parse a comment line
// It should be called when a ";" is encountered, and it is expected to keep
// parsing until a newline or line-feed character is reached, and return 0.
// If a null character is reached, it will return 2. It returns 1 or error,
// which should only happen in weird circumstances. It advances the pointer
// of the string being parsed to the next character, unless it reached nullchar.
//

int inEDN_ParseComment( struct inEDN_state_s* es )
{
   if( es == NULL ) {
      fprintf( stdout, "Error: null EDN pointer \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Parsing __Comment__ \n" );
#endif

   int ierr=0;

   const char* ptr = es->ptr;
#ifdef _DEBUG_COMMENT2_
   if(fp) fprintf( fp, " [DEBUG2]  Starting pointer at -->%c<-- \n", ptr[0] );
#endif
   while( ierr == 0 ) {
      // seek a newline
      if( ptr[0] == '\n' || ptr[0] == '\r') {
#ifdef _DEBUG_COMMENT2_
         if(fp) fprintf( fp, "\n" );
#endif
         ++ptr;
         ierr = 100;
      } else if( ptr[0] == '\0' ) {
#ifdef _DEBUG_COMMENT2_
         if(fp) fprintf( fp, "\n" );
#endif
         ierr = 102;      // truncated; not necessarily a problem/error
      } else {
#ifdef _DEBUG_COMMENT2_
         if(fp) fprintf( fp, "%c", ptr[0] );
#endif
         ++ptr;
      }
   }

   es->ptr = ptr;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Ending parsing of __Comment__ \n" );
#endif
   if( ierr == 100 ) return 0;
   if( ierr == 102 ) return 2;
}


//
// Function to parse "Whitespace" (exhausts whitespace or fails at truncation)
//

int inEDN_ParseWhitespace( struct inEDN_state_s* es )
{
   if( es == NULL ) {
      fprintf( stdout, "Error: null EDN pointer \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Parsing __Whitespace__ \n" );
#endif

   int ierr=0;
   const char* ptr = es->ptr;
#ifdef _DEBUG_WHITE2_
   if(fp) fprintf( fp, " [DEBUG2]  Starting pointer at -->%c<-- \n", ptr[0] );
#endif
   while( ierr == 0 ) {
      // spaces, tabs, and newlines prior to the colon
      if( ptr[0] == ' ' || ptr[0] == '\t' ) {
         ++ptr;
      } else if( ptr[0] == ',' ) {   // lingering comma is a whitespace (what?)
         ++ptr;
      } else if( ptr[0] == '\0' ) {
         ierr = 101;  // truncated; not necessarily a problem/error
      } else {
         ierr = 100;  // located something
      }
   }
   es->ptr = ptr;

   if( ierr == 101 ) {
#ifdef _DEBUG_WHITE2_
      if(fp) fprintf( fp, " [DEBUG2]  Went past whitespace with truncation\n" );
#endif
      return 2;
   }
   if( ierr == 100 ) {
#ifdef _DEBUG_WHITE2_
      if(fp) fprintf( fp, " [DEBUG2]  Went past whitespace -->%c<--\n", ptr[0]);
#endif
      return 0;
   }
}


//
// function to parse a numeric item (int or real)
//

int inEDN_ParseNumeric( struct inEDN_state_s* es,
                        union inEDN_type_u* u )
{
   if( es == NULL || u == NULL ) {
      fprintf( stdout, "Error: null EDN/u pointer \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Parsing __Numeric__ \n" );
#endif

   int ierr=0;
   const char* ptr = es->ptr;
#ifdef _DEBUG_NUMERIC2_
   if(fp) fprintf( fp, " [DEBUG2]  Starting pointer at -->%c<-- \n", ptr[0] );
#endif
   unsigned char iperiod=0, itype=0, isuffix=0;
   // size the numeric item (to allocate a string)
   // determine if it is a real or integer
   // determine if it has offensive characters or spaces, etc
   int n=0;
   while( ierr == 0 ) {
      if( ptr[n] == '+' || ptr[n] == '-' ) {
         // allow signs
      } else if( ptr[n] == '.' ) {
         ++iperiod;   // counting periods
         if( iperiod == 2 ) ierr=103;    // error: too many periods
         // check for sanity (can have "N" prior to "." in error)
         if( itype == 1 ) {
            ierr=104;    // mixing integers and reals
         } else {
            itype=2;
         }
      } else if( ptr[n] == 'e' || ptr[n] == 'E' ) {
         // allow for exponent-mantissa reals
         // check for sanity (can "e"/"E" trail "N" in error)
         if( itype == 1 ) {
            ierr=104;    // mixing integers and reals
         } else {
            itype=2;
         }
      } else if ( ptr[n] == 'N' ) {
         // allow trailing "N" for ints, but check for characters of a real
         ++isuffix;
         if( isuffix == 2 ) ierr=103;    // error: too many Ns
         if( itype == 2 ) {
            ierr=104;    // mixing integers and reals
         } else {
            itype=1;
         }
      } else if( 48 <= ptr[n] && ptr[n] <= 57 ) {
         // allow decimal digits
      } else if ( ptr[n] == '\n' || ptr[n] == '\r' ) {
         ierr=102;    // truncated by newline; may not be an error
         --n;   // retract a byte
      } else if ( ptr[n] == '\0' ) {
         ierr=102;    // truncated by nullchar; may not be an error
         --n;   // retract a byte
      } else if ( ptr[n] == ' ' || ptr[n] == '\t' || ptr[n] == ',' ||
                  ptr[n] == ')' || ptr[n] == ']' || ptr[n] == '}' ||
                  ptr[n] == '(' || ptr[n] == '[' || ptr[n] == '{' ) {
         ierr=100;    // terminated in a reasonable fashion
         --n;   // retract a byte
      } else {
#ifdef _DEBUG_NUMERIC2_
         if(fp) fprintf( fp, " [DEBUG2]  Alpha character in error \n" );
#endif
         ierr=101;    // error: all other alpha characters is in error
      }

      ++n;
   }
#ifdef _DEBUG_NUMERIC2_
   if(fp) fprintf( fp, " [DEBUG2]  Counted %d bytes; state: %d \n", n, ierr );
#endif

   if( ierr == 100 || ierr == 102 ) {
      if( itype == 0 ) itype=1;

      char* str = (char*) malloc( (size_t) n+1 );
      if( str == NULL ) {
         if(fp) fprintf( fp, " [Error]  memory allocation problem \n" );
         return -1;
      }
      memcpy( str, ptr, (size_t) n );
      str[n] = '\0';
#ifdef _DEBUG_NUMERIC2_
      if(fp) fprintf( fp, " [DEBUG2]  Payload \"%s\"\n", str );
#endif
      if( isuffix ) {
         str[n-1] = '\0';
#ifdef _DEBUG_NUMERIC2_
         if(fp) fprintf( fp, " [DEBUG2]  Has suffix N; clean \"%s\" \n", str );
#endif
      }

      // parse and re-assign if no error
      if( itype == 1 ) {
         long l = atol( str );
         struct inEDN_int_s* p = (struct inEDN_int_s*) u;
         p->type = inEDN_Type_Int;
         p->l = l;
#ifdef _DEBUG_NUMERIC2_
         if(fp) fprintf( fp, " [DEBUG2]  Changed to \"int\" %ld \n", p->l );
#endif
      } else if( itype == 2 ) {
         double d = strtod( str, NULL );
         struct inEDN_real_s* p = (struct inEDN_real_s*) u;
         p->type = inEDN_Type_Real;
         p->d = d;
#ifdef _DEBUG_NUMERIC2_
         if(fp) fprintf( fp, " [DEBUG2]  Changed to \"real\" %16.9e \n", p->d );
#endif
      } else {
         // should errors occur (not possible now), we revert everything
         ierr=999;
      }
      free( str );

#ifdef _DEBUG_NUMERIC2_
      if(fp) fprintf( fp, " [DEBUG2]  Advancing by %d bytes \n", n );
#endif
#ifdef _DEBUG_NUMERIC3_
      if(fp) fprintf( fp, " [DEBUG2]  Start pointer at \"%c\" \n", es->ptr[0] );
#endif
      es->ptr += (size_t) (n-0);
#ifdef _DEBUG_NUMERIC3_
      if( es->ptr[0] == '\n' || es->ptr[0] == '\r' )
         if(fp) fprintf( fp, " [DEBUG3]  End pointer at \"\\n / \\r\" \n" );
      else
         if(fp) fprintf( fp, " [DEBUG3]  End pointer at \"%c\"\n", es->ptr[0] );
#endif
   }

   if( ierr == 100 ) ierr=0;
   if( ierr == 102 ) ierr=2;
   if( ierr == 101 || ierr == 103 || ierr == 104 ) ierr=3;

#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Ending parsing of __Numeric__ (%d)\n", ierr );
#endif

   return ierr;
}


//
// function to parse the string stored under a "symbol" type, expecting that
// it is a numeric value (real or int)
//

int inEDN_ParseNumericFromSymbol( struct inEDN_state_s* es,
                                  struct inEDN_symbol_s* sp )
{
   if( es == NULL || sp == NULL ) {
      fprintf( stdout, "Error: null EDN/sp pointer \n");
      return 1;
   }
   if( sp->size <= 0 ) {
      fprintf( stdout, "Error: numeric string of zero size \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Parsing __NumericSymbol__ \n" );
   if(fp) fprintf( fp, " [DEBUG]  Payload of symbol -->%s<---\n", sp->string );
#endif

   int ierr=0;
   char* ptr = sp->string;
   unsigned char itype=0, isuffix=0;

   // determine if it has a suffix "N" to indicate precision (and ignore it)
   if( ptr[ sp->size-1 ] == 'N' ) {
#ifdef _DEBUG_NUMERIC2_
      if(fp) fprintf( fp, " [DEBUG2]  Has suffix N \n" );
#endif
      isuffix=1;    // to reverse the following change
      ptr[ sp->size-1 ] = '\0';
      itype=1;      // it must be an integer by convention
   }

   // determine if it is a fraction or integer, and...
   // determine if it has offensive characters or spaces, etc
   if( itype == 0 ) itype=1;      // assume it is an integer
   int n=0;
   while( ptr[n] != '\0' && ierr == 0 ) {
      if( ptr[n] == '.' ) itype=2;
      // check for errors; set "itype=0" and "ierr!=0" on error
      if( ptr[n] == '+' || ptr[n] == '-' ) {
         // signs
      } else if( ptr[n] == 'e' || ptr[n] == 'E' ) {  // fine
         itype=2;
      } else if( 48 <= ptr[n] && ptr[n] <= 57 ) {
         // digits
      } else {
         itype=0;
         ierr=1;
      }
      ++n;
   }

   // parse and re-assign if no error
   if( itype == 1 ) {
      long l = atol( ptr );
      free( sp-> string );
      struct inEDN_int_s* p = (struct inEDN_int_s*) sp;
      p->type = inEDN_Type_Int;
      p->l = l;
#ifdef _DEBUG_NUMERIC2_
      if(fp) fprintf( fp, " [DEBUG2]  Changed to \"int\" %ld \n", p->l );
#endif
   } else if( itype == 2 ) {
      double d = strtod( ptr, NULL );
      free( sp-> string );
      struct inEDN_real_s* p = (struct inEDN_real_s*) sp;
      p->type = inEDN_Type_Real;
      p->d = d;
#ifdef _DEBUG_NUMERIC2_
      if(fp) fprintf( fp, " [DEBUG3]  Changed to \"real\" %16.9e \n", p->d );
#endif
   } else {
      // should errors occur (not possible now), we revert everything
      ierr=100;
      if( isuffix ) ptr[ sp->size-1 ] = 'N';
   }

#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Ending parsing of __NumericSymbol__ \n" );
#endif

   return ierr;
}


//
// function to parse a string
// (A string is enclosed in double quotes.)
//

int inEDN_ParseString( struct inEDN_state_s* es,
                       struct inEDN_string_s* sp )
{
   if( es == NULL || sp == NULL ) {
      fprintf( stdout, "Error: null EDN/sp pointer \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Parsing __String__ \n" );
#endif

   int ierr=0;
   const char* ptr = es->ptr;
#ifdef _DEBUG_STRING2_
   if(fp) fprintf( fp, " [DEBUG2]  Starting pointer at -->%c<--\n", ptr[0] );
#endif
   size_t size=0;
   unsigned char iesc=0;
   while( ierr == 0 ) {
      ++ptr;

      // seek an un-escaped double quote
      if( ptr[0] == '\"' ) {
         if( iesc ) {
#ifdef _DEBUG_STRING3_
            if(fp) fprintf( fp, "\"" );
#endif
            ++size;
         } else {
            ierr = 100;      // end of string
         }
         iesc=0;
      } else if( ptr[0] == '\\' ) {
#ifdef _DEBUG_STRING3_
         if(fp) fprintf( fp, "\\" );
#endif
         if( iesc ) { iesc=0; } else { iesc=1; }
         ++size;
      } else if( ptr[0] == '\0' ) {
#ifdef _DEBUG_STRING2_
         if(fp) fprintf( fp, "\n [Error]  String is truncated! \n" );
#endif
         ierr = 102;      // truncated
      } else {
         iesc=0;
#ifdef _DEBUG_STRING3_
         if(fp) fprintf( fp, "%c", ptr[0] );
#endif
         // "characters" need to be parsed here as UTF-8, but...
         // ...we do not concern ourselves with this right now!
         ++size;
      }
   }
   if( ierr == 102 ) return 2;
#ifdef _DEBUG_STRING3_
   if( ierr == 100 ) if(fp) fprintf( fp, "\n" );
#endif

#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  String size is %ld bytes \n", size );
#endif
   sp->string = (char*) malloc( size+1 );
   if( sp->string == NULL ) {
      if(fp) fprintf( fp, " [Error]  Memory allocation failed for string \n" );
      return -1;
   }
   memcpy( sp->string, (void*) &(es->ptr[1]), size );
   sp->string[size] = '\0';
   sp->size = size;

   es->ptr = ptr + 1;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Ending parsing of __String__ \n" );
   if(fp) fprintf( fp, " [DEBUG]  Payload -->%s<---\n", sp->string );
#endif

   return 0;
}


//
// Function to parse a "Boolean"
// (I have a feeling this will change... We do not need a boolean if we have
// to fully parse for it to begin with...)
//

int inEDN_ParseBoolean( struct inEDN_state_s* es,
                        struct inEDN_bool_s* bp )
{
   if( es == NULL || bp == NULL ) {
      fprintf( stdout, "Error: null EDN/bp pointer \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Parsing __Boolean__ \n" );
#endif

   int ierr=0;
   const char* ptr = es->ptr;
   unsigned char ic=0xff,nc=0;
#ifdef _DEBUG_BOOLEAN2_
   if(fp) fprintf( fp, " [DEBUG]  Starting pointer at -->%c<-- \n", ptr[0] );
#endif
   while( ierr == 0 ) {
      ++ptr;

      if( ic == 0xff ) {
         if( ptr[0] == 't' || ptr[0] == 'T' ) ic=1;
         if( ptr[0] == 'f' || ptr[0] == 'F' ) ic=0;
         if( ic == -1 ) ierr=101;    // unexpected character
      }
      if( ic == 0 ) {
         switch( nc ) {
          case 1:
            if( ptr[0] != 'r' && ptr[0] != 'R' ) ierr = 101;
          break;
          case 2:
            if( ptr[0] != 'u' && ptr[0] != 'U' ) ierr = 101;
          break;
          case 3:
            if( ptr[0] != 'e' && ptr[0] != 'E' ) ierr = 101; else ierr=100;
          break;
          default:
            ierr=101;   // unexpected character
         }
      }
      if( ic == 1 ) {
         switch( nc ) {
          case 1:
            if( ptr[0] != 'a' && ptr[0] != 'A' ) ierr = 101;
          break;
          case 2:
            if( ptr[0] != 'l' && ptr[0] != 'L' ) ierr = 101;
          break;
          case 3:
            if( ptr[0] != 's' && ptr[0] != 'S' ) ierr = 101;
          break;
          case 4:
            if( ptr[0] != 'e' && ptr[0] != 'E' ) ierr = 101; else ierr=100;
          break;
          default:
            ierr=101;   // unexpected character
         }
      }
      // over-write errors in case of truncation
      if( ptr[0] == '\0' ) ierr=102;   // detect truncated file

      ++nc;
   }

   if( ierr == 102 ) {
      if(fp) fprintf( fp, " [Error]  Boolean is truncated \n" );
      return 2;
   }

   if( ierr == 101 ) {
      if(fp) fprintf( fp, " [Error]  Boolean is garbled \n" );
      return 1;
   }

   bp->b = ic;
   es->ptr = ptr + 1;

#ifdef _DEBUG_BOOLEAN2_
   if(fp) fprintf( fp, " [DEBUG]  Pointer in boolean \"%c\"\n", ptr[0] );
#endif
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Went past boolean \n" );
#endif
   return 0;
}


//
// function to parse a symbol (is string-like)
// (A symbol has restrictions! We presently do not check for much...)
// Returns 0 on success, 1 and -1 on errors, and 2 when nullchar is reached.
//

int inEDN_ParseSymbol( struct inEDN_state_s* es,
                       struct inEDN_symbol_s* sp )
{
   if( es == NULL || sp == NULL ) {
      fprintf( stdout, "Error: null EDN/sp pointer \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Parsing __Symbol__ \n" );
#endif

   int ierr=0;
   const char* ptr = es->ptr;
#ifdef _DEBUG_SYMBOL2_
   if(fp) fprintf( fp, " [DEBUG2]  Starting pointer at -->%c<-- \n", ptr[0] );
#endif
   size_t size=1;      // symbol includes starting character
   while( ierr == 0 ) {
      ++ptr;

      // seek whitespace, newline, or closing delimiters to end parsing
      if( ptr[0] == ' ' || ptr[0] == ',' || ptr[0] == '\t' ) {
         ierr = 100;      // end of symbol
      } else if( ptr[0] == '\n' || ptr[0] == '\r' ) {
         ierr = 100;      // end of symbol by reaching end of line
      } else if( ptr[0] == ')' || ptr[0] == ']' || ptr[0] == '}' ) {
         // This may not be a thing that happens; whitespace makes sense, but
         // we tolerate reaching end of container.
         ierr = 100;      // end of symbol
      } else if( ptr[0] == '(' || ptr[0] == '[' || ptr[0] == '{' ) {
         // This may not be a thing that happens; whitespace makes sense, but
         // we tolerate reaching start of container.
         ierr = 100;      // end of symbol
      } else if( ptr[0] == '\\' ) {
         ierr = 101;      // unexpected character
#ifdef _DEBUG_SYMBOL2_
         if(fp) fprintf( fp, " [DEBUG2]  Unexpected backslash \n" );
#endif
      } else if( ptr[0] == '\0' ) {
         ierr = 102;      // truncated; this may not be a problem, tolerate it
#ifdef _DEBUG_SYMBOL2_
         if(fp) fprintf( fp, " [DEBUG2]  Nullchar; caller may tolerate it\n" );
#endif
      } else {
         ++size;
      }
   }

   if( ierr == 101 ) return 3;

#ifdef _DEBUG_SYMBOL2_
   if(fp) fprintf( fp, " [DEBUG2]  Symbol size is %d bytes \n", size );
#endif
   sp->string = (char*) malloc( size+1 );
   if( sp->string == NULL ) {
      if(fp) fprintf( fp, " [Error]  Memory allocation failed for symbol \n" );
      return -1;
   }
   memcpy( sp->string, (void*) &(es->ptr[0]), size );
   sp->string[size] = '\0';
   sp->size = size;

   es->ptr = ptr;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Ending parsing of __Symbol__ \n" );
   if(fp) fprintf( fp, " [DEBUG]  Payload -->%s<---\n", sp->string );
#endif
#ifdef _DEBUG_SYMBOL3_
   if(fp) fprintf( fp, " [DEBUG3]  Pointer at \"%c\" (Symbol) \n", es->ptr[0] );
#endif

   if( ierr == 102 ) ierr=2;    // let the caller handle a truncated stream
   if( ierr == 100 ) ierr=0;
   return ierr;
}


//
// function to re-examine the string of a parsed symbol candidate
// and potentially reset the type with deeper parsing
//

int inEDN_ReparseSymbol( struct inEDN_state_s* es,
                         struct inEDN_symbol_s* sp )
{
   if( es == NULL || sp == NULL ) {
      fprintf( stdout, "Error: null EDN/sp pointer \n");
      return 1;
   }
   if( sp->size <= 0 ) {
      fprintf( stdout, "Error: string of zero size \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Re-parsing __Symbol__ candidate \n" );
   if(fp) fprintf( fp, " [DEBUG]  Payload \"%s\"\n", sp->string );
#endif

   int ierr=0;
   const char* ptr = sp->string;
#ifdef _DEBUG_SYMBOL2_
   if(fp) fprintf( fp, " [DEBUG2]  Starting pointer at -->%c<-- \n", ptr[0] );
#endif
   // check if this is numeric; starts with a sign
   if( ptr[0] == '+' || ptr[0] == '-' ) {
#ifdef _DEBUG_SYMBOL2_
      if(fp) fprintf( fp, " [DEBUG2]  Sign detected \n" );
#endif
      ++ptr;
      if( 48 <= ptr[0] && ptr[0] <= 57 ) {
#ifdef _DEBUG_SYMBOL2_
         if(fp) fprintf( fp, " [DEBUG2]  Numeric detected; delegating \n" );
#endif
         // delegate further parsing... (pointer is at the sign)
         ierr = inEDN_ParseNumericFromSymbol( es, sp );
      }
   }

#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Ending re-parsing of __Symbol__ \n" );
#endif

   return ierr;
}


//
// Function to parse a "List" and a "vector"
//

// forward declaration
int inEDN_ParsePound( struct inEDN_state_s* es, union inEDN_type_u* p );
int inEDN_ParseMap( struct inEDN_state_s* es, struct inEDN_map_s* map );
// definitions for functions that are re-used
#define inEDN_ParseVector  inEDN_ParseList
#define inEDN_ParseSet     inEDN_ParseList

int inEDN_ParseList( struct inEDN_state_s* es,
                     struct inEDN_list_s* list )
{
   if( es == NULL || list == NULL ) {
      fprintf( stdout, "Error: null EDN/list-vector-set pointer \n");
      return 1;
   }
   if( list->type != inEDN_Type_List &&
       list->type != inEDN_Type_Vector &&
       list->type != inEDN_Type_Set ) {
      fprintf( stdout, "Error: bad delimiter \n");
      return 1;
   }
   char delim = ')';
   if( list->type == inEDN_Type_Vector ) delim = ']';
   if( list->type == inEDN_Type_Set ) delim = '}';

   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) if( delim == ')' ) fprintf( fp, " [DEBUG]  Parsing __List__ \n" );
          else if( delim == '}' ) fprintf( fp, " [DEBUG]  Parsing __Set__ \n" );
          else fprintf( fp, " [DEBUG]  Parsing __Vector__ \n" );
#endif

   int ierr=0;
#ifdef _DEBUG_LIST2_
   if(fp) fprintf( fp, " [DEBUG2]  Starting pointer at -->%c<--\n", es->ptr[0]);
#endif
   ++( es->ptr );
   while( ierr == 0 ) {
      const char* ptr = es->ptr;
#ifdef _DEBUG_LIST3_
      if(fp) fprintf( fp, " [DEBUG2]  Pointer inside list \"%c\"\n", ptr[0] );
#endif

      if( ptr[0] == delim ) {
         // end the list/vector/set
#ifdef _DEBUG_LIST2_
         if(fp) fprintf( fp, " [DEBUG2]  Reached end with: \"%c\"\n", ptr[0] );
#endif
         ++( es->ptr );
         ierr = 100;
      } else if( ptr[0] == '\0' ) {
         ierr = 101;  // truncated
      } else if( ptr[0] == '\n' || ptr[0] == '\r' ) {   // newlines
         // tolerate
         ++( es->ptr );
      } else if( ptr[0] == ' ' || ptr[0] == '\t' || ptr[0] == ',' ) {
         // tolerate whitespace right here
#ifdef _DEBUG_LIST2_
         if(fp) fprintf( fp, " [DEBUG2]  Whitespace in list/vector \n" );
#endif
         ++( es->ptr );
      } else if( ptr[0] == ']' || ptr[0] == '}' || ptr[0] == ')' ) {
         // (this is a generic stop that _follows_ the proper termination above)
         ierr = 103;  // misplaced terminator
      } else {
         // at this point we will need to extend; do it and check for errors
         size_t isize = (list->num + 1) * sizeof( union inEDN_type_u );
         union inEDN_type_u* p = (union inEDN_type_u*)
                           realloc( list->data, isize );
         if( p == NULL ) {
            if(fp) fprintf( fp, " [Error]  Failed reallocating array \n" );
            ierr=-1;
         } else {
#ifdef _DEBUG_LIST2_
            if(fp) fprintf( fp, " [DEBUG2]  Extended array (%d) \n", list->num);
#endif
            list->data = p;
            // classify the segment
            ierr = inEDN_ClassifyToken( es, ptr[0] );
            if( ierr < 2000 ) {
               printf("Crazy error in parsing list!\n");//HACK
               exit(1);
            }
            p = &( list->data[ (list->num)++ ] );
         }

         // react to parse segment
         switch( ierr ) {
          case -1:
            // let the error flow through
          break;
          case 2001:
          { struct inEDN_list_s* list = (struct inEDN_list_s*) p;
            INEDN_LIST_INIT( *list )
            ierr = inEDN_ParseList( es, list );
          } // detect result and re-set ierr
          break;
          case 2002:
          { struct inEDN_vector_s* vector = (struct inEDN_vector_s*) p;
            INEDN_VECTOR_INIT( *vector )
            ierr = inEDN_ParseVector( es, (struct inEDN_list_s*) vector );
          } // detect result and re-set ierr
          break;
          case 2003:
          { struct inEDN_map_s* map = (struct inEDN_map_s*) p;
            INEDN_MAP_INIT( *map )
            ierr = inEDN_ParseMap( es, map );
          } // detect result and re-set ierr
          break;
          case 3001:
            INEDN_STRING_INIT( *((struct inEDN_string_s*) p) )
            ierr = inEDN_ParseString( es, (struct inEDN_string_s*) p );
            if( ierr == 2 ) ierr = 102;
          break;
          case 4000:
            // classify the dispatch character
            ierr = inEDN_ClassifyPound( es, ptr[0] );
            if( ierr <= 4000 ) {
               printf("Crazy error in parsing (at dispatch)!\n");//HACK
               exit(1);
            }
            ierr = inEDN_ParsePound( es, p );
            /// check for error
            printf("Not checking for errors in parsing pound dispatch\n");//HACK
          break;
          case 5001:
            p->boolean.type = inEDN_Type_Bool;
            p->boolean.b = 1;
            es->ptr += 4;
            ierr=0;
#ifdef _DEBUG_LIST2_
            if(fp) fprintf( fp, " [DEBUG2]  List item: \"true\" \n" );
#endif
          break;
          case 5002:
            p->boolean.type = inEDN_Type_Bool;
            p->boolean.b = 0;
            es->ptr += 5;
            ierr=0;
#ifdef _DEBUG_LIST2_
            if(fp) fprintf( fp, " [DEBUG2]  List item: \"false\" \n" );
#endif
          break;
          case 5003:
            p->nil.type = inEDN_Type_Nil;
            es->ptr += 3;
            ierr=0;
#ifdef _DEBUG_LIST2_
            if(fp) fprintf( fp, " [DEBUG2]  List item: \"nil\" \n" );
#endif
          break;
          case 6000:
#ifdef _DEBUG_LIST2_
            if(fp) fprintf( fp, " [DEBUG]  List processing \"symbol\"\n" );
#endif      
            ierr = inEDN_ParseSymbol( es, (struct inEDN_symbol_s*) p );
            if( ierr ) {
               printf("Need to deicde what to do on this error 1 \n");//HACK
               ierr = 102;    // HACK decide what to do on "truncation'
            } else {
               p->symbol.type = inEDN_Type_Symbol;
               // check for the symbol being something else
               ierr = inEDN_ReparseSymbol( es, (struct inEDN_symbol_s*) p );
               if( ierr ) {
                  printf("Need to deicde what to do on this error 2 \n");//HACK
                  ierr = 102;    // HACK decide what to do
               }
            }
          break;
          case 7000:
#ifdef _DEBUG_LIST2_
            if(fp) fprintf( fp, " [DEBUG]  List to parse as \"symbol\"\n" );
#endif
            ierr = inEDN_ParseSymbol( es, (struct inEDN_symbol_s*) p );
            if( ierr ) {
               printf("Need to deicde what to do on this error 1 \n");//HACK
               ierr = 102;    // HACK decide what to do on "truncation'
            } else {
               struct inEDN_keyword_s* pk = (struct inEDN_keyword_s*) p;
               pk->type = inEDN_Type_Keyword;
            }
          break;
          case 8000:
#ifdef _DEBUG_LIST2_
            if(fp) fprintf( fp, " [DEBUG]  List processing \"numeric\"\n" );
#endif
            ierr = inEDN_ParseNumeric( es, p );
            if( ierr ) {
               if( ierr == 2 ) {   // acceptable within a list
                  ierr=0;
               }
            } else {
               // I am guessing it went through...
            }
          break;
          default:
#ifdef _DEBUG_LIST2_
            if(fp) fprintf( fp, " [DEBUG]  List reached default \n" );
#endif
            // this should be an error
            ierr = 999;
          break;
         }
#ifdef _DEBUG_LIST2_
         if(fp) fprintf( fp, " [DEBUG2]  Advancing (delim \"%c\")\n", delim );
#endif
      }
#ifdef _DEBUG_STEP_
      usleep(100000);
#endif
   }

   if( ierr == -1 ) return -1;
   if( ierr == 103 ) {
#ifdef _DEBUG_LIST2_
      if(fp) fprintf( fp, " [DEBUG]  Misplaced terminator detected! \n" );
#endif
      return 4;
   }
   if( ierr == 102 ) {
#ifdef _DEBUG_LIST2_
      if(fp) fprintf( fp, " [DEBUG]  Item is truncated\n" );
#endif
      return 3;
   }
   if( ierr == 101 ) {
#ifdef _DEBUG_LIST2_
      if(fp) fprintf( fp, " [DEBUG]  Went past lst/vec/set with truncation\n" );
#endif
      return 2;
   }
   if( ierr == 100 ) {
#ifdef _DEBUG_LIST2_
      if(fp) if( delim == ')' ) {
         fprintf( fp, " [DEBUG2]  Went past __List__ \n" );
      } else if(fp) if( delim == ']' ) {
         fprintf( fp, " [DEBUG2]  Went past __Vector__ \n" );
      } else {
         fprintf( fp, " [DEBUG2]  Went past __Set__ \n" );
      }
      if( list->num == 0 ) {
         if(fp) fprintf( fp, " [DEBUG2]  Empty? Weird... \n" );
      }
#endif
      return 0;
   }
   return 999;    // a default error code while developing
}


//
// Function to parse a "dispatch character" item
// (Ths is just a delegator in the middle...)
//

int inEDN_ParsePound( struct inEDN_state_s* es,
                      union inEDN_type_u* p )
{
   if( es == NULL || p == NULL ) {
      fprintf( stdout, "Error: null EDN/type pointer \n");
      return 1;
   }

/// Get one character; fail on error
/// Determine to what this is dispatched (most likely a set)
/// Call to parse a set



   return 999;
}


/*******************************************************************
//
// Function to extend a "Map" type's contents by a new pair
//

int inEDN_ExtendMap( struct inEDN_state_s *es,
                     struct inEDN_map_s *map )
{
   if( es == NULL || map == NULL ) return 1;

   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Extending Object at %p, num. of items: %ld\n",
            map, map->num );
#endif

   size_t isize = sizeof(struct inEDN_pair_s);
   struct inEDN_pair_s *p = (struct inEDN_pair_s*) malloc( isize );
   if( p == NULL ) {
      sprintf( es->msg, "Error: could not allocate new Pair " );
      if(fp) fprintf( fp, "%s\n", es->msg );
      return -1;
   }
   INEDN_PAIR_INIT( *p );

   if( map->tail == NULL ) {
      map->head = p;
      map->tail = p;
   } else {
      p->prev = map->tail;
      map->tail->next = p;
      map->tail = p;
   }
   map->num += 1;

   return 0;
}
 *******************************************************************/


//
// Function to parse a Map
//

int inEDN_ParseMap( struct inEDN_state_s* es,
                    struct inEDN_map_s* map )
{
   if( es == NULL || map == NULL ) {
      fprintf( stdout, "Error: null EDN/map pointer \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Parsing a __Map__ \n" );
#endif
#ifdef _DEBUG_MAP2_
   if(fp) fprintf( fp, " [DEBUG2]  Starting pointer at \"%c\" \n", es->ptr[0] );
#endif

   struct inEDN_set_s tmpset;
   INEDN_SET_INIT( tmpset );
   int ierr = inEDN_ParseSet( es, (struct inEDN_list_s*) &tmpset );
   if( ierr ) {
      if(fp) fprintf( fp, " [Error]  Parsing Map as Set failed \n" );
      return ierr;
   }

#ifdef _DEBUG_MAP3_
   if(fp) fprintf( fp, " [DEBUG3]  Array read for the map %d \n", tmpset.num );
   inEDN_UnrollList( es, (struct inEDN_list_s*) &tmpset, 10 );
#endif

   if( tmpset.num == 0 ) {
#ifdef _DEBUG_MAP2_
      if(fp) fprintf( fp, " [DEBUG2]  Map is empty \n" );
#endif
      return 0;
   }
   if( tmpset.num % 2 ) {
      if(fp) fprintf( fp, " [Error]  Bad key-value pairings in map \n" );
      return 10;
   }

   map->data = (struct inEDN_pair_s*)
              malloc( ((size_t) tmpset.num/2) * sizeof(struct inEDN_pair_s) );
   union inEDN_type_u* tmp = (union inEDN_type_u*)
              malloc( ((size_t) tmpset.num) * sizeof(union inEDN_type_u) );
   if( map->data == NULL || tmp == NULL ) {
      if( map->data != NULL ) free( map->data );
      if( tmp != NULL ) free( tmp );

///// THE FOLLOWING WILL CHANGE; I WILL DUMP THE SET'S CONTENTS (HACK)
      // return the items read; caller will do a clean-up...
      memcpy( map, &tmpset, sizeof(union inEDN_type_u) );

      return -1;
   }

   map->num = tmpset.num/2;
   for(size_t n=0;n<tmpset.num/2;++n) {
      INEDN_PAIR_INIT( map->data[n] )
      map->data[n].key = &( tmp[2*n] );
      map->data[n].value = &( tmp[2*n+1] );
      memcpy( map->data[n].key, &( tmpset.data[2*n] ),
              sizeof(union inEDN_type_u) );
      memcpy( map->data[n].value, &( tmpset.data[2*n+1] ),
              sizeof(union inEDN_type_u) );
   }

///// HERE I NEED TO CLEAN THE tmpset VARIABLE OF ALL ATTACHED MEMORY

#ifdef _DEBUG_MAP2_
   if(fp) fprintf( fp, " [DEBUG]  Went past map \n" );
#endif
   return ierr;
}


//
// The main EDN object/data parsing function
// The toplevel object for the EDN format does not exist; concatataion of many
// types is allowed. This function needs my construct of a toplevel type, which
// is an array of the generic (union) type that keeps getting extended.
//

int inEDN_Parse( struct inEDN_state_s* es,
                 union inEDN_type_u* edn_ )
{
   if( es == NULL || edn_ == NULL ) {
      fprintf( stdout, "Error: null EDN/edn pointer \n");
      return 1;
   }
   FILE *fp = es->fp;
#ifdef _DEBUG_
   if(fp) fprintf( fp, " [DEBUG]  Parsing EDN at top level \n" );
#endif

   struct inEDN_toplevel_s* edn = (struct inEDN_toplevel_s*) edn_;
   INEDN_TOPLEVEL_INIT( *edn )

   int ierr=0;
#ifdef _DEBUG2_
   if(fp) fprintf( fp, " [DEBUG2]  Starting pointer at \"%c\" \n", es->ptr[0] );
#endif
   while( ierr == 0 ) {
      const char* ptr = es->ptr;
#ifdef _DEBUG2_
      if(fp) {
         if( ptr[0] == '\n' || ptr[0] == '\r' )
            fprintf( fp, " [DEBUG2]  Pointer at toplevel -->(\\n / \\r)<--\n" );
         else if( ptr[0] == '\0' )
            fprintf( fp, " [DEBUG2]  Pointer at toplevel -->(\\0)<--\n" );
         else
            fprintf( fp, " [DEBUG2]  Pointer at toplevel -->%c<--\n", ptr[0] );
      }
#endif
      if( ptr[0] == '\0' ) {
         // a termination condition; needs no extending
         ierr=100;
      } else if( ptr[0] == '\n' || ptr[0] == '\r' ) {
         // keep parsing
         ++(es->ptr);
      } else if( ptr[0] == ';' ) {
         // a comment needs no extending
         ierr = inEDN_ParseComment( es );
         if( ierr == 2 ) {    // the case where the EDN was null-terminated
            // this is an acceptable termination condition
            ierr=100;
         }
      } else if( ptr[0] == ' ' || ptr[0] == '\t' || ptr[0] == ',' ) {
         ierr = inEDN_ParseWhitespace( es );
         // the only possible error is finding a truncated string
         if( ierr ) ierr=100;
      } else {
         // all other tokens need extending followed by classification
#ifdef _DEBUG_
         if(fp) fprintf( fp, " [DEBUG]  Toplevel extending \n" );
#endif
         union inEDN_type_u* u=NULL;
         ierr = inEDN_Extend( es, edn );
         if( ierr ) {
            // deal with the error...
            ierr = -1;    // This is the only error possible, trust me!
         } else {
            u = &( edn->array[ edn->num-1 ] );
            ierr = inEDN_ClassifyToken( es, ptr[0] );
         }

         // after extending the top-level chain and having classified, parse it
         switch( ierr ) {
          case -1:        // a memory allocation error occured while extending
            // do nothing for now
          break;
          case 1000:      // a truncated file _in_error_
            ierr = 999;
          break;
          case 2001:      // a list
          { struct inEDN_list_s* list = (struct inEDN_list_s*) u;
            INEDN_LIST_INIT( *list )
            ierr = inEDN_ParseList( es, list );
          } // detect result and re-set ierr
          break;
          case 2011:      // the (misplaced) end of a list
            printf("Misplaced end-of-list \n"); // HACK
            ierr=999;     // error-out for now
          break;
          case 2002:      // a vector
          { struct inEDN_vector_s* vector = (struct inEDN_vector_s*) u;
            INEDN_VECTOR_INIT( *vector )
            ierr = inEDN_ParseVector( es, (struct inEDN_list_s*) vector );
          } // detect result and re-set ierr
          break;
          case 2012:      // the (misplaced) end of a vector
            printf("Misplaced end-of-vector \n"); // HACK
            ierr=999;     // error-out for now
          break;
          case 2003:      // a map
          { struct inEDN_map_s* map = (struct inEDN_map_s*) u;
            INEDN_MAP_INIT( *map)
            ierr = inEDN_ParseMap( es, map );
          } // detect result and re-set ierr
          break;
          case 2013:      // the (misplaced) end of a map
            printf("Misplaced end-of-map \n"); // HACK
            ierr=999;     // error-out for now
          break;
          case 3001:      // a string
          { struct inEDN_string_s* string = (struct inEDN_string_s*) u;
            INEDN_STRING_INIT( *string )
            ierr = inEDN_ParseString( es, string );
          } // detect result and re-set ierr
          break;
          case 4000:
            // classify the dispatch character
            ++ptr;
            ierr = inEDN_ClassifyPound( es, ptr[0] );
            if( ierr <= 4000 ) {
               printf("Crazy error in parsing (at dispatch)!\n");//HACK
               exit(1);
            }
            ierr = inEDN_ParsePound( es, u );
            // detect result and re-set ierr
          break;
          case 5001:
            u->boolean.type = inEDN_Type_Bool;
            u->boolean.b = 1;
            es->ptr += 4;
            ierr=0;
#ifdef _DEBUG_
            if(fp) fprintf( fp, " [DEBUG]  Toplevel \"true\" \n" );
#endif
          break;
          case 5002:
            u->boolean.type = inEDN_Type_Bool;
            u->boolean.b = 0;
            es->ptr += 5;
            ierr=0;
#ifdef _DEBUG_
            if(fp) fprintf( fp, " [DEBUG]  Toplevel \"false\" \n" );
#endif
          break;
          case 5003:
            u->nil.type = inEDN_Type_Nil;
            es->ptr += 3;
            ierr=0;
#ifdef _DEBUG_
            if(fp) fprintf( fp, " [DEBUG]  Toplevel \"nil\" \n" );
#endif
          break;
          case 6000:      // a symbol
#ifdef _DEBUG_
            if(fp) fprintf( fp, " [DEBUG]  Toplevel processing \"symbol\"\n" );
#endif
            ierr = inEDN_ParseSymbol( es, (struct inEDN_symbol_s*) u );
            if( ierr == 0 || ierr == 2 ) {  // "nullchar" may be tolerated
               u->symbol.type = inEDN_Type_Symbol;
               // check for the symbol being something else
               ierr = inEDN_ReparseSymbol( es, (struct inEDN_symbol_s*) u );
               if( ierr == 0 || ierr == 2 ) {  // "nullchar" may be tolerated
                  ierr=0;
               } else {
                  ierr=999;     // error-out until we decide what to do HACK
               }
            } else {
               ierr=999;     // error-out until we decide what to do HACK
            }
          break;
          case 7000:      // a keyword
#ifdef _DEBUG_
            if(fp) fprintf( fp, " [DEBUG]  Toplevel to parse as \"symbol\"\n" );
#endif
            ierr = inEDN_ParseSymbol( es, (struct inEDN_symbol_s*) u );
            if( ierr == 0 || ierr == 2 ) {  // "nullchar" may be tolerated
               u->nil.type = inEDN_Type_Keyword;
            } else {
               ierr=999;     // error-out until we decide what to do HACK
            }
          break;
          case 8000:      // a numeric (may be integer or real)
#ifdef _DEBUG_
            if(fp) fprintf( fp, " [DEBUG]  Toplevel processing \"numeric\"\n" );
#endif
            ierr = inEDN_ParseNumeric( es, u );
            if( ierr == 0 || ierr == 2 ) {  // "nullchar" may be tolerated
               ierr=0;
            } else {
               ierr=999;     // error-out until we decide what to do HACK
            }
          break;
          default:
            // this should be an error
            ierr = 999;
         }

         // advance
         if( !ierr ) {
#ifdef _DEBUG_
            if(fp) fprintf( fp, " [DEBUG]  Advancing at toplevel \n" );
#endif
         } else {
#ifdef _DEBUG_
            if(fp) fprintf( fp, " [DEBUG]  Error returned; no advancing. \n" );
            if(fp) fprintf( fp, " [DEBUG]  Got: ierr=%d \n", ierr );
#endif
         }
      }
#ifdef _DEBUG_STEP_
      usleep(100000);
#endif
   }

   if( ierr == -1 ) {
      if(fp) fprintf( fp, " [Error]  memory allocation problem \n" );
   } else if( ierr == 100 ) {
#ifdef _DEBUG_
      if(fp) fprintf( fp, " [DEBUG]  Parsing EDN ended \n" );
#endif
      ierr=0;
   } else if( ierr != 0 ) {  // this is temporary, to allow me to build first...
      // indicate that this is a massive fiasco
      ierr=1;
   }

   return ierr;
}

