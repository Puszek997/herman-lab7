override CFLAGS=-std=c17 -Wall -Wextra -Wshadow -Wvla -Wno-unused-parameter -Wno-unused-const-variable -g -O0 -fsanitize=address,undefined,leak

ifdef CI
override CFLAGS=-std=c17 -Wall -Wextra -Wshadow -Wvla -Werror -Wno-unused-parameter -Wno-unused-const-variable
endif

NAME=sop-werona

.PHONY: clean all

all: ${NAME}

${NAME}: ${NAME}.c l7-common.h
	$(CC) $(CFLAGS) -o ${NAME} ${NAME}.c

clean:
	rm -f ${NAME}
