#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int ptrBoundCheck(unsigned long start, unsigned long end, unsigned long val) {
	if (val < start || val > end) {
		fprintf(stderr, "oob!!\n");
		exit(-1);
	}
}

int read_chars(char *ptr, size_t length) {
	read(0, ptr, length);
}

int print_chars(char *ptr, size_t length) {
	write(1, ptr, length);
}

char read_char() {
	char a;
	read(0, &a, sizeof(a));
	return a;
}

int print_char(char b) {
	char c = b;
	write(1, &c, sizeof(c));
	return 1;
}