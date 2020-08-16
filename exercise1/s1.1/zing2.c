#include <stdio.h>
#include <unistd.h>
void zing(void)
{
printf("2nd version of: Hello, %s \n", getlogin());
}
