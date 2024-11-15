lib_src+=../tool/dump.c
lib_src+=../library/bintp1.c

clang_flag+=-I../include/
clang_flag+=-I../tool/
clang_flag+=-O0
clang_flag+=-g

default: basic

basic: ${lib_src} ./basic.c
	clang ${lib_src} ./basic.c \
		--output basic \
		${clang_flag}

performance: ${lib_src} ./performance.c
	clang ${lib_src} ./performance.c \
		--output performance \
		${clang_flag}
