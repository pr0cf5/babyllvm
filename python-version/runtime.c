#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct DATA_s {
	char *data_ptr;
	char *start_ptr;
	char buf[0x3000];
};

struct DATA_s DATA;

int ptrBoundCheck(unsigned long start, unsigned long end, unsigned long val) {
	if (val < start || val > end) {
		fprintf(stderr, "assert (0x%lx < 0x%lx < 0x%lx)!!\n", start, val, end);
		exit(-1);
	}
}

void *alloc_data() {
	memset(DATA.buf, 0x0, 0x3000);
	return &DATA;
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
