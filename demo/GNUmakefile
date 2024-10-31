source+=./main.c
source+=../library/request.c
source+=../library/memory.c

default: main

main: ${source}
	clang ${source} \
		--output main \
		-I../library/ \
		-g
