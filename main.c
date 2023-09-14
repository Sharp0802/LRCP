#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <ncurses.h>
#include <ctype.h>

#include "cio.h"

#define boffset(p, n) (void*)((size_t)(void*)p + (n))

#define PPRC_SYN 0x01
#define PPRC_ACK 0x02
#define PPRC_FIN 0x04
#define PPRC_RST 0x08
#define PPRC_CWR 0x10

struct __attribute__((aligned(1))) pprc_h
{
    unsigned char flg;
    unsigned char usrn;
};

struct rcv_c
{
    int fd;
    WINDOW* wnd;
    struct sockaddr_in csa;
};

int csocket(unsigned int ip, unsigned short port)
{
    struct sockaddr_in ssa;
    struct timeval tv;
    int fd;
    
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!fd) goto EXCEPT;
    
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
	perror("setsockopt");
	goto EXCEPT;
    }
    
    ssa.sin_family = AF_INET;
    ssa.sin_addr.s_addr = ip;
    ssa.sin_port = htons(port);
    
    if (bind(fd, (const struct sockaddr*)&ssa, sizeof(ssa)) < 0)
    {
	perror("bind");
	goto EXCEPT;
    }
    
    return fd;
EXCEPT:
    if (fd) close(fd);
    return 0;
}

ssize_t mkpk(void* buf, size_t n, unsigned char flg, const char* usr, const char* msg)
{
    struct pprc_h h;
    size_t ul = strlen(usr);
    size_t ml = strlen(msg);
    size_t o = 0;
    if (n < ul + ml + sizeof h)
	return -1;
    
    h.flg = flg;
    h.usrn = ul;
    
    memcpy(boffset(buf, o), &h, sizeof h);
    o += sizeof h;
    memcpy(boffset(buf, o), usr, ul);
    o += ul;
    memcpy(boffset(buf, o), msg, ml);
    o += ml;
    
    if (o >> (sizeof o - 1))
    {
	errno = E2BIG;
	return -1;
    }
    
    return (ssize_t)o;
}

void* receiver(void* t)
{
    struct rcv_c ctx = *(struct rcv_c*)t;
    
    char* usr = getlogin();
    int fd = ctx.fd;
    WINDOW* wnd = ctx.wnd;
    char* buf = malloc(BUFSIZ);
    char* pkbuf = boffset(buf, BUFSIZ / 2);
    
    ssize_t rcv;
    struct pprc_h h;
    struct sockaddr_in csa;
    socklen_t len;
    char* pusr = malloc(64);
    unsigned char usrcn = 64;
    
    for (;;)
    {
	rcv = recvfrom(fd, pkbuf, BUFSIZ / 2, MSG_DONTWAIT, (struct sockaddr*)&csa, &len);
	if (rcv < 0)
	{
	    if (errno == EAGAIN)
		continue;
	    wperror(wnd, "recvfrom");
	    break;
	}
	
	if ((size_t)rcv < sizeof h)
	{
	    errno = EBADMSG;
	    wperror(wnd, "receiver");
	    continue;
	}
	
	h = *(struct pprc_h*)pkbuf;
	if (h.flg & PPRC_FIN)
	{
	    buf[0] = 0;
	    rcv = mkpk(pkbuf, BUFSIZ / 2, PPRC_FIN, usr, buf);
	    rcv = sendto(fd, pkbuf, rcv, MSG_CONFIRM, (const struct sockaddr*)&csa, len);
	    if (rcv == -1)
	    {
		rcv = mkpk(pkbuf, BUFSIZ / 2, PPRC_RST, usr, buf);
		rcv = sendto(fd, pkbuf, rcv, MSG_CONFIRM, (const struct sockaddr*)&csa, len);
	    }
	    if (rcv == -1)
	    {
		wperror(wnd, "sendto");
	    }
	    wprintw(wnd, "[!!] Connection finished by peer\n");
	    break;
	}
	else if (h.flg & PPRC_RST)
	{
	    wprintw(wnd, "[!!] Connection reset by peer\n");
	    break;
	}
	else
	{
	    if (h.usrn + 1 > usrcn)
	    {
		void* p = realloc(pusr, h.usrn + 1);
		if (!p)
		{
		    wprintw(wnd, "realloc");
		    break;
		}
		pusr = p;
		usrcn = h.usrn + 1;
	    }
	    
	    memcpy(pusr, boffset(pkbuf, sizeof h), h.usrn);
	    pusr[h.usrn] = 0;
	    
	    wprintw(wnd, "[%s@%s] %s\n",
		    pusr,
		    inet_ntoa(csa.sin_addr),
		    (char*)boffset(pkbuf, sizeof h + h.usrn));
	}
    }
    
    pthread_exit(NULL);
}

int gui(unsigned int ip, unsigned short dp, unsigned short sp)
{
    char* usr = getlogin();
    char* buf = malloc(BUFSIZ);
    char* pkbuf = boffset(buf, BUFSIZ / 2);
    int y, x;
    WINDOW* ownd, * iwnd, * ifwnd;
    
    struct sockaddr_in ssa, csa;
    int fd;
    ssize_t rcv;
    
    pthread_t rx;
    struct rcv_c rc;
    
    /* INIT GUI */
    
    initscr();
    cbreak();
    nonl();
    noecho();
    curs_set(0);
    
    getmaxyx(stdscr, y, x);
    
    ownd = subwin(stdscr, y - 5, x, 0, 0);
    iwnd = subwin(stdscr, 3, x, y - 3, 0);
    ifwnd = subwin(iwnd, 1, x - 2, y - 2, 1);
    
    wborder(iwnd, '|', '|', '-', '-', '+', '+', '+', '+');
    
    keypad(ifwnd, TRUE);
    wtimeout(ifwnd, 0);
    intrflush(ifwnd, 0);
    leaveok(ifwnd, TRUE);
    
    wrefresh(iwnd);
    wrefresh(ownd);
    wrefresh(ifwnd);
    
    /* INIT SOCKET */
    
    ssa.sin_family = AF_INET;
    ssa.sin_addr.s_addr = INADDR_ANY;
    ssa.sin_port = htons(sp);
    
    csa.sin_family = AF_INET;
    csa.sin_addr.s_addr = ip;
    csa.sin_port = htons(dp);
    
    fd = csocket(INADDR_ANY, sp);
    if (!fd)
    {
	wperror(ownd, "csocket");
	goto FINALIZE;
    }
    
    rc.fd = fd;
    rc.wnd = ownd;
    rc.csa = csa;
    
    /* MAIN LOOP */
    
    pthread_create(&rx, NULL, receiver, &rc);
    
    do
    {
	wrefresh(ownd);
	wgetnstrnb(ifwnd, buf, BUFSIZ / 2);
	wclear(ifwnd);
	wprintw(ownd, "[%s@%s] %s\n", usr, inet_ntoa(ssa.sin_addr), buf);
	
	rcv = mkpk(pkbuf, BUFSIZ / 2, PPRC_ACK, usr, buf);
	if (rcv < 0)
	{
	    wperror(ownd, "mkpk");
	    continue;
	}
	
	rcv = sendto(fd, pkbuf, rcv, MSG_CONFIRM, (const struct sockaddr*)&csa, sizeof csa);
	if (rcv < 0)
	{
	    if (errno == EAGAIN)
		continue;
	    wperror(ownd, "sendto");
	    goto FINALIZE;
	}
	
	wrefresh(ownd);
    } while (strcmp(buf, ":q") != 0);
    
    /* FINALIZE */
    
    buf[0] = 0;
    rcv = mkpk(pkbuf, BUFSIZ / 2, PPRC_FIN, usr, buf);
    rcv = sendto(fd, pkbuf, rcv, MSG_CONFIRM, (const struct sockaddr*)&csa, sizeof csa);
    if (rcv == -1)
    {
	rcv = mkpk(pkbuf, BUFSIZ / 2, PPRC_RST, usr, buf);
	sendto(fd, pkbuf, rcv, MSG_CONFIRM, (const struct sockaddr*)&csa, sizeof csa);
    }
    
    pthread_join(rx, NULL);
    
    /* CLEANUP */

FINALIZE:
    wprintw(ownd, "[!!] press any key to exit...");
    wrefresh(ownd);
    
    getch();
    
    endwin();
    free(buf);
    
    return 0;
}

int main(int argc, char* argv[])
{
    unsigned int ip;
    unsigned short dp, sp;
    int r;
    
    if (argc < 2)
	goto E_SYNTAX;
    
    switch (argv[1][0])
    {
    case 'c':
	if (argc < 4)
	    goto E_SYNTAX;
	
	r = 0;
	break;
    case 'd':
	if (argc < 5)
	    goto E_SYNTAX;
	
	inet_pton(AF_INET, argv[2], &ip);
	dp = strtoul(argv[3], NULL, 10);
	sp = strtoul(argv[4], NULL, 10);
	r = gui(ip, dp, sp);
	break;
    case 's':
	r = 0;
	break;
    default:
	goto E_SYNTAX;
    }
    
    return r;
E_SYNTAX:
    puts("invalid syntax.");
    return 1;
}
