/* ELMO helper function declarations.
 * See elmoasmfunctions.s for implementations. */

#ifndef ELMOASMFUNCTIONSDEF_H
#define ELMOASMFUNCTIONSDEF_H

extern void starttrigger(void);
extern void endtrigger(void);
extern void randbyte(unsigned char *pointer);
extern void LoadN(void *addr);
extern void readbyte(unsigned char *pointer);
extern void printbyte(unsigned char *pointer);
extern void endprogram(void);
extern void getstart(unsigned int *pointer);
extern void getruncount(unsigned int *pointer);

#endif
