#include "util/PrintException.hxx"
#include "CgroupProc.hxx"

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
try {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s PID\n", argv[0]);
		return EXIT_FAILURE;
	}

	printf("%s\n", ReadProcessCgroup(atoi(argv[1]), "").c_str());
	return EXIT_SUCCESS;
} catch (const std::exception &e) {
	PrintException(e);
	return EXIT_FAILURE;
}
