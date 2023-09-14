#include <stdlib.h>

extern int main(int argc, char* argv[]);

__asm__(".text \n"
	".global _start \n"
	"_start: \n"
	"	xor %rbp,%rbp \n"
	"	mov %rsp,%rdi \n"
	".weak _DYNAMIC \n"
	".hidden _DYNAMIC \n"
	"	lea _DYNAMIC(%rip),%rsi \n"
	"	andq $-16,%rsp \n"
	"	call _start_c \n"
	);

void _start_c(long* p)
{
    exit(main((int)*p, (void*)(p + 1)));
}
