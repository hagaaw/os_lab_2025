#include "swap.h"

void Swap(char *left, char *right)
{
	char newchar = *left;
	*left = *right;
	*right = newchar;
}
