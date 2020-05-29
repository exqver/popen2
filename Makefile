CC	= gcc $(CFLAGS)
CFLAGS	= -g -Wall
INCLUDE	=
LIBS	=

O	= out
D	= $O/.dep

CFILES	= popen2.c main.c
OFILES	= $(CFILES:%.c=$O/%.o)
DFILES	= $(CFILES:%.c=$D/%.c.dep)

TARGET	= test-popen2

.PHONY: clean all

all: $O/$(TARGET)

$O/$(TARGET): $(OFILES)
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CC) -o $@ $(OFILES) $(LIBS)

$O/%.o: %.c
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CC) -o $@ -c $<

clean:
	$(RM) -rf $O

$D/%.c.dep: %.c
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CC) $(DEFINE) $(INCLUDE) -MF $@ -MT "$O/$*.o $@ TAGS" -M $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DFILES)
endif


