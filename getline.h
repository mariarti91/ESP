/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   getline.h
 * Author: Nikita
 *
 * Created on June 10, 2017, 8:27 AM
 */

#include <stdio.h>
#include <string.h>


#define OK       0
#define NO_INPUT 1
#define TOO_LONG 2
#ifndef GETLINE_H
#define GETLINE_H

#ifdef __cplusplus
extern "C" {
#endif


int getLine (char *prmpt, char *buff, size_t sz);


int getLine (char *prmpt, char *buff, size_t sz) {
    int ch, extra;

    // Get line with buffer overrun protection.
    if (prmpt != NULL) {
        printf ("%s", prmpt);
        fflush (stdout);
    }
    if (fgets (buff, sz, stdin) == NULL)
        return NO_INPUT;

    // If it was too long, there'll be no newline. In that case, we flush
    // to end of line so that excess doesn't affect the next call.
    if (buff[strlen(buff)-1] != '\n') {
        extra = 0;
        while (((ch = getchar()) != '\n') && (ch != EOF))
            extra = 1;
        return (extra == 1) ? TOO_LONG : OK;
    }

    // Otherwise remove newline and give string back to caller.
    buff[strlen(buff)-1] = '\0';
    return OK;
}


#ifdef __cplusplus
}
#endif

#endif /* GETLINE_H */

