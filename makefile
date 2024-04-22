PROGRAM = http_server

CPPFLAGS = $(PROGRAM).h
CFLAGS = -g
RELFLAGS = -DNDEBUG -O3
GD = gcc -ansi -pedantic-errors -Wall -Wextra

debug: $(PROGRAM)
$(PROGRAM): $(PROGRAM).c $(PROGRAM)_test.c
	@$(GD) $(CPPFLAGS) $(CFLAGS) $^ -o $@

run:
	./$(PROGRAM)

gdb:
	gdb $(PROGRAM)

vlg:
	valgrind --leak-check=yes --track-origins=yes ./$(PROGRAM)