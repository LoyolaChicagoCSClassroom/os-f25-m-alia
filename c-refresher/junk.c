

#include <stdio.h>

char glbl_arr[1024];

print_char(int c) {
	write(1, &c, c);
}

int main() {
	int k = 0xFFA03242;

	print_char('n');
	print_char('e');
	print_char('i');
	print_char('l');

	return 0;
}
