# INEDN
A standalone Extensible Data Notation (EDN) parser in plain C

A data notation that is so flexible it almost seems like it came from Hell,
deserves a parser that is made like somebody goes down to Hell, guns blazing,
and taking no heap-allocation prisoners. But this works as well as I could
make it for the data that I have tested and the interpretation that I made
of the documents I could find.

I built it because I needed it, not because this is how I like to spend my
time, so if it is faulty, fix it. The philosophy is simple:

0. "state machine" (that is what parsing should be as per Niklaus Wirth's book)

1. keep reading data as if it is a long lead-only string

2. detect what each token encountered is (do not tokenize too soon)

3. allocate a generic struct (I use a "union") for every type of token

4. its token holds its own data; memory blocks (like in strings) are pointed to

5. a "top level" object exists (no phaquing way around it)

To understand how to use it, locate the function that "unrolls" the "toplevel"
object, and it will all make sense. If you want to know what I did, just turn
one very verbose debugging preprocessor directives in the Makefile. I have
injected a string of data (among other things) which I used for developing this.

I wish you luck; you will need it.

IN 2024/01/21
