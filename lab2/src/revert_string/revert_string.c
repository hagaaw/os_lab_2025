#include <string.h>
#include "revert_string.h"
void RevertString(char *str)
{
	int i = 0;
	int j = strlen(str) - 1;

	while(i < j){
		char temp = str[i];
		str[i] = str[j];
		str[j] = temp;
		i++;
		j--;
	}

}

