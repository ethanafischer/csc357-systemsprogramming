#include <stdio.h>
#include <ctype.h>

int main(int argc, char *argv[]) {
	FILE *fr;
	int lines = 0;
	int words = 0;
	int bytes = 0;
	int in_word = 0; // track if we are inside a word

	if (argc > 1) {
		fr = fopen(argv[1], "r");
	} else {
		printf("Enter file contents:\n");
		fr = stdin;
	}

	int c = fgetc(fr);
	while (c != EOF){
		bytes++;
		if(c == '\n'){
			lines++;
		}
		if (isspace(c)) {
			if (in_word) {
				words++;
				in_word = 0;
			}
		} else {
			in_word = 1;
		}
		c = fgetc(fr);
	}
	if (in_word) {
		words++;
	}
	printf("%d %d %d\n", lines, words, bytes);
	if (fr != stdin) {
		fclose(fr);
	}

	return 0;
}
