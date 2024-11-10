source+=./main.c
source+=../library/bintp1.c

default: main

main: ${source}
	clang ${source} \
		--output main \
		-I../library/ \
		-O0 \
		-g
