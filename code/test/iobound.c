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
    int i, j;
    
    for(i = 0; i < 100; i++)
    {
        PrintInt(i);
    } 
    // Halt();
    /* not reached */
}
