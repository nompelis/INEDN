"; This is a comment\n"
"; Trying to write a parser that makes sense\n"
"\n\n\r"  // a sequence of newlines
//"true"
//"false"
//"nil"
//"\0"  // a NULL-termination that should end parsing correctly
//"  ( )"   // a list
//"  ( < > )"   // "symbols" for quick debugging
//"  ( -junk $ADA github/code    *)"   // "symbols" for quick debugging
//"  (if )"   // "symbols" for quick debugging
  " nil "     // "boolean" for quick debugging
//"true "     // "boolean" for quick debugging
//"false"     // "boolean" for quick debugging
//" 1.2 "     // "numeric" for quick debugging
//":mykeyword"   // "keyword"
  "< "     // "symbol" for quick debugging
//"(   "  // WRAP IN LIST
//"-321"      // "integer" for quick debugging
//" 4321N "   // "integer" for quick debugging
//",876 "     // "integer" for quick debugging
//"8a76 "     // "integer" for quick debugging (triggers error)
//" \"a\\\"\" "     // "string" injected in numeric list... for debugging
//" \"a    \" "     // "string" injected in numeric list... for debugging
//" \" \"\"x\" "     // tight "string" injected in numeric list... for debugging
//":mykey "   // "keyword" for quick debugging
//"nil "      // "nil" for quick debugging
//" true "    // "boolean" for quick debugging
//"9876"      // "integer" for quick debugging
  " 7.7E-7"   // "double" for quick debugging
//")   "  // WRAP IN LIST
//" ( ( ( ( true nil false ) ) ) )"  // ALL AT ONCE
//"-111N "     // "integer" for quick debugging
//"-2.2e-9  "     // "real" for quick debugging
//"  ( . * + ! - _ ? $ % & = < > )"   // "symbols"
//"  ( true    false nil)"   // a list with booleans
//"  [ true    false ]"   // a vector with booleans
  "  { \"x\"   false 40 nil }"   // a map
//"  ( ]"   // a list that is mis-terminated! ERROR
//"  ( )"   // a 2nd list
//"  { CRAP MAP}"
//"  [ CRAP IN HERE] \n"  // vector
//"  [ \"CRAP\" \"IN\" \"HERE\"] \n"  // vector of strings
//"  ( \"CRAP\" \"IN\" \"HERE OR \\\"THERE\\\"\") \n"  // list of strings
//"\"STRING\""
//"TEST"
//" true false nil "   // plain specials for debugging
//")"  // a misplaced list terminator for debugging; used ] and } as well
//"; Comment on the same line \n"
//"; End of file"
;
