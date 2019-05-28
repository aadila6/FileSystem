# Target library
lib := libfs.a
CFLAGS := -Wall -Werror -g 
all: $(lib)

$(lib): disk.o fs.o
	ar rcs $(lib) $^ -l

disk.o: disk.c disk.h
	gcc $(CFLAGS) $< -c

fs.o: fs.c fs.h
	gcc $(CFLAGS) $< -c

clean: 
	rm -rf $(lib) disk.o fs.o

## TODO: Phase 1


