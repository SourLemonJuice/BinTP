source+=./main.c
source+=../library/bintp1.c
source+=../library/memory.c

default: main

main: ${source}
	clang ${source} \
		--output main \
		-I../library/ \
		-g
