all:
	gcc -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wall -Werror -Wextra -D_GNU_SOURCE=1 -o memccli -std=gnu99 -lmemcached memccli.c

format:
	clang-format -i memccli.c

.PHONY: all format
