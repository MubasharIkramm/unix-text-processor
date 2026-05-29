CC   = gcc -std=gnu99	# Use gcc for Zeus
OPTS = -Og -Wall -Werror -Wno-error=unused-variable -Wno-error=unused-function -Wformat-truncation=0 
DEBUG = -g					# -g for GDB debugging

UID := $(shell id -u)
SRCDIR=./src
OBJDIR=./obj
INCDIR=./inc
BINDIR=.

INCLUDE=$(addprefix -I,$(INCDIR))
HEADERS=$(wildcard $(INCDIR)/*.h)
CFLAGS=$(OPTS) $(INCLUDE) $(DEBUG)
OBJECTS=$(addprefix $(OBJDIR)/,textmngr.o logging.o parse.o util.o)

all: textmngr slow_cooker my_echo slow_jojo

textmngr: $(OBJECTS)
	$(CC) $(CLAGS) -o $@ $^

$(OBJDIR)/textmngr.o: $(SRCDIR)/textmngr.c $(INCDIR)/textmngr.h
	$(CC) $(CFLAGS) -c $< -o $@
#	gcc -D_POSIX_C_SOURCE -Wall -g -std=c99 -c textproc.c   

$(OBJDIR)/parse.o: $(SRCDIR)/parse.c $(INCDIR)/parse.h
	$(CC) $(CFLAGS) -c $< -o $@     

$(OBJDIR)/util.o: $(SRCDIR)/util.c $(INCDIR)/util.h
	$(CC) $(CFLAGS) -c $< -o $@     

$(OBJDIR)/logging.o: $(SRCDIR)/logging.c $(INCDIR)/logging.h
	$(CC) $(CFLAGS) -c $< -o $@


slow_cooker: $(SRCDIR)/slow_cooker.c
	$(CC) $(CFLAGS) -o slow_cooker $<

my_echo: $(SRCDIR)/my_echo.c
	$(CC) $(CFLAGS) -o my_echo $<

slow_jojo: $(SRCDIR)/slow_jojo.c
	$(CC) $(CFLAGS) -o slow_jojo $<

clean:
	rm -rf $(OBJDIR)/*.o textmngr my_pause slow_cooker my_echo slow_jojo




