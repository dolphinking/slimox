CFLAGS =  -Wall -g3 -O3 -mmmx -msse -msse2 -fno-strict-aliasing
LDFLAGS = -Wall -g3 -O3 -mmmx -msse -msse2 -lpthread

SRCDIR:= src
BINDIR:= bin
OBJDIR:= obj

DIR:= $(shell mkdir -p $(OBJDIR) $(BINDIR))
SRC:= $(shell echo src/*.c)
OBJ:= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(notdir $(SRC)))))
DEP:= $(addprefix $(OBJDIR)/, $(addsuffix .d, $(basename $(notdir $(SRC)))))
BIN:= bin/slimox

$(BIN): $(OBJ)
	@ echo -e " linking..."
	@ $(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
	@ echo -e " done."

-include $(DEP)

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	@ echo -n -e " generating makefile dependence of $<..."
	@ $(CC) $(CFLAGS) -MM $< | sed "s?\\(.*\\):?$(OBJDIR)/$(basename $(notdir $<)).o $(basename $(notdir $<)).d :?g" > $@
	@ echo -e " done."

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@ echo -n -e " compiling $<..."
	@ $(CC) $(CFLAGS) -c -o $@ $<
	@ echo -e " done."

clean:
	@ echo -n -e " cleaning..."
	@ rm -rf $(DEP) $(OBJ) $(BIN)
	@ rmdir -p --ignore-fail-on-non-empty $(BINDIR)
	@ echo -e " done."

.IGNORE: clean
.PHONY:  clean
