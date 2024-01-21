/******************************************************************************
 This is IN's implementation of an EDN parser.
 Built by changing the JSON parser.

 2023/12/28 IN <nompelis@nobelware.com> - Initial creation
 2024/01/10 IN <nompelis@nobelware.com> - Last modification
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "inedn.h"

static char global_edn_text[] =
#include "METADATA.c"


//
// Function to read a file as text
//

int read_textfile( const char filename[], char** text )
{
   if( filename == NULL || text == NULL ) return 1;

   int ierr=0;
   struct stat fs;
   ierr = stat( filename, &fs );
   if( ierr != 0 ) {
      fprintf( stdout, "ERROR: could not stat() file \"%s\" \n", filename );
      return 2;
   }

   *text = (char*) malloc( (size_t) fs.st_size + 1 );
   if( *text == NULL ) {
      fprintf( stdout, "ERROR: could not allocate %ld bytes \n",
               (long int) fs.st_size );
      return 3;
   }

   int fd = open( filename, O_RDONLY );
   if( fd == -1 ) {
      fprintf( stdout, "ERROR: could not open file \"%s\" \n", filename );
      return 4;
   }

   if( read( fd, *text, (size_t) fs.st_size ) < (ssize_t) fs.st_size ) {
      fprintf( stdout, "ERROR: could not read entire file! \n" );
      close( fd );
      return 5;
   }

   close( fd );

   return 0;
}


//
// Driver
//

int main( int argc, char *argv[] )
{
   char* edn_text = global_edn_text;

   if( argc == 2 ) read_textfile( argv[1], &edn_text );

   printf(" EDN text: \n ============== \n%s\n =========== \n", edn_text );

   union inEDN_type_u edn;
   INEDN_TYPE_INIT( edn )

   struct inEDN_state_s es;
   es.fp = stdout;
   es.ptr = edn_text;

   // testing parsing of the whole EDN block...
   if( !inEDN_Parse( &es, (union inEDN_type_u*) &edn ) ) {
      (void) inEDN_UnrollToplevel( &es, (struct inEDN_toplevel_s*) &edn );
   }

   return 0;
}

