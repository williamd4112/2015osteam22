/* add.c
 *	Simple program to test whether the systemcall interface works.
 *
 *	Just do a add syscall that adds two values and returns the result.
 *
 */

#include "syscall.h"

int
main()
{
    int result = 0;
    int i;
    
    while(result < 20)
        result++;
    PrintInt(result);

    for(i = 0; i < 20; i++)
    {
        int j = 0;
        while(j != 20) 
            j++;
            Yield();
    }
    PrintInt(result);
    /* not reached */
}
