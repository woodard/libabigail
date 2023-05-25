/*
   Compile this file twice.  Once with DWARF debug info and once with
   CTF debug info:

   gcc -g -c test-PR29811-unknown-size-array-dwarf-ctf.c -o test-PR29811-unknown-size-array-dwarf-ctf-DWARF.o

  gcc -gctf -c test-PR29811-unknown-size-array-dwarf-ctf.c -o test-PR29811-unknown-size-array-dwarf-ctf-CTF.o

*/
extern unsigned int is_basic_table[];

unsigned int is_basic_table[] = {0};
