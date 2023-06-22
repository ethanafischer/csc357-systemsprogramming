#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
	FILE *f;
	char l1[513];
	char l2[513] = "";

	if(argc > 1) {
		f = fopen(argv[1], "r");
	} else {
		f = stdin;
	}

	while(fgets(l1, sizeof(l1), f) != NULL) {
		if (strcmp(l1, l2) != 0) {
			printf("%s", l1);
			strcpy(l2, l1);
		}
	}

	if(f != stdin) {
		fclose(f);
	}
	return 0;
}
