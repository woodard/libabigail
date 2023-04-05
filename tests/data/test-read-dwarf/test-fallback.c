/* gcc -gctf -c test-fallback.c -o test-fallback.o

   This test case is meant to test the fallback feature in
   libabigail tools, so when those tools are executed without
   the explicit use of `--ctf' option it should be use CTF
   frond-end to build the IR.
 */
int a;
