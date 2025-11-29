#include <stdio.h>

int main(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i)
	{
		fprintf(stderr, "argv[%d] = '%s'\n", i, argv[i]);
	}
	return 0;
}
