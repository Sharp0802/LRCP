#ifndef PPRC_CIO_H
#define PPRC_CIO_H

#include <ctype.h>
#include <ncurses.h>
#include <string.h>

#define wperror(w, s) wprintw(w, "[!!] " s ": %s\n", strerror(errno))

void wrdrbuf(WINDOW* w, const char* buf, size_t n, size_t cur, size_t* lr);
size_t wgetnstrnb(WINDOW* w, char* buf, size_t n);

#endif
