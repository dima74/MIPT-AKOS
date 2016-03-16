rm -f 1.o
rm -f a.out
gcc -c 1.c

ld -dynamic-linker /usr/local/lib/gcc/x86_64-unknown-linux-gnu/5.3.0/crtbegin.o /lib64/ld-linux-x86-64.so.2 /usr/lib/x86_64-linux-gnu/crt1.o /usr/lib/x86_64-linux-gnu/crti.o -lc 1.o --no-as-needed /usr/local/lib/gcc/x86_64-unknown-linux-gnu/5.3.0/crtend.o /usr/lib/x86_64-linux-gnu/crtn.o
#ld -dynamic-linker /usr/local/lib/gcc/x86_64-unknown-linux-gnu/5.3.0/crtbegin.o /lib64/ld-linux-x86-64.so.2 /usr/lib/x86_64-linux-gnu/crt1.o /usr/lib/x86_64-linux-gnu/crti.o -lc 1.o /usr/local/lib/gcc/x86_64-unknown-linux-gnu/5.3.0/crtend.o /usr/lib/x86_64-linux-gnu/crtn.o

#/lib64/ld-linux-x86-64.so.2 
#/usr/lib/x86_64-linux-gnu/crt1.o 
#/usr/lib/x86_64-linux-gnu/crti.o 
#/usr/local/lib/gcc/x86_64-unknown-linux-gnu/5.3.0/crtbegin.o

#/lib64/ld-linux.so.2 
#/usr/lib/x86_64-linux-gnu/crt1.o 
#/usr/lib/x86_64-linux-gnu/crti.o


#ld /lib64/ld-linux-x86-64.so.2 /lib/x86_64-linux-gnu/libc.so.6 /usr/lib/x86_64-linux-gnu/crt1.o /usr/lib/x86_64-linux-gnu/crti.o -lc 1.o
#ld /lib64/ld-linux-x86-64.so.2 /lib/x86_64-linux-gnu/libc.so.6 /usr/lib/x86_64-linux-gnu/crt1.o -lc 1.o
#ld /lib64/ld-linux-x86-64.so.2 /usr/lib/x86_64-linux-gnu/crt1.o /usr/lib/x86_64-linux-gnu/crti.o -lc 1.o /usr/lib/x86_64-linux-gnu/crtn.o
#ld -dynamic-linker /lib64/ld-linux.so.2 /usr/lib/x86_64-linux-gnu/crt1.o /usr/lib/x86_64-linux-gnu/crti.o -lc 1.o /usr/lib/x86_64-linux-gnu/crtn.o
