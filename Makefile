 CC = gcc
 COPTS = -Wall -g
 COPTS =       -g

 DEBUG += -D  _DEBUG_
#DEBUG += -D  _DEBUG2_
#DEBUG += -D  _DEBUG3_

#DEBUG += -D  _DEBUG_STEP_
 DEBUG += -D  _DEBUG_CLASSIFY2_ -D  _DEBUG_CLASSIFY3_
#DEBUG += -D  _DEBUG_COMMENT2_
#DEBUG += -D  _DEBUG_WHITE2_
#DEBUG += -D  _DEBUG_BOOLEAN2_
#DEBUG += -D  _DEBUG_LIST2_ -D  _DEBUG_LIST3_
#DEBUG += -D  _DEBUG_STRING2_
#DEBUG += -D  _DEBUG_SPECIAL2_
#DEBUG += -D  _DEBUG_SYMBOL2_ -D  _DEBUG_SYMBOL3_
#DEBUG += -D  _DEBUG_NUMERIC2_ -D  _DEBUG_NUMERIC3_
#DEBUG += -D  _DEBUG_MAP2_ -D  _DEBUG_MAP3_

all:
	$(CC) -c $(COPTS) $(DEBUG) inedn.c
	$(CC)    $(COPTS) $(DEBUG) main.c \
                              inedn.o
clean:
	rm -f *.o a.out

