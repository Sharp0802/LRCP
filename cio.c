#include "cio.h"

void wrdrbuf(WINDOW* w, const char* buf, size_t n, size_t cur, size_t* lr)
{
    size_t i, r;
    for (i = 0; i < n; ++i)
    {
	chtype ch = (chtype)buf[i];
	if (i == cur)
	    ch |= A_REVERSE;
	mvwaddch(w, 0, i, ch);
    }
    if (i == cur)
    {
	chtype ch = ' ';
	ch |= A_REVERSE;
	mvwaddch(w, 0, i, ch);
	i++;
    }
    r = i;
    for (; i < *lr; ++i)
	mvwaddch(w, 0, i, ' ');
    *lr = r;
}

size_t wgetnstrnb(WINDOW* w, char* buf, size_t n)
{
    int t = 1;
    size_t l = 0, lr = 0;
    size_t cur = 0;
    chtype ch;
    
    for (; t;)
    {
	wrdrbuf(w, buf, l, cur, &lr);
	ch = wgetch(w);
	
	if (!(ch & KEY_CODE_YES) && isprint(ch))
	{
	    buf[cur++] = (char)ch;
	    if (++l >= n)
		break;
	}
	
	switch (ch)
	{
	case ERR:
	    break;
	
	case KEY_LEFT:
	    if (cur > 0)
		cur--;
	    break;
	case KEY_RIGHT:
	    if (cur < l)
		cur++;
	    break;
	case KEY_HOME:
	    cur = 0;
	    break;
	case KEY_END:
	    cur = l - 1;
	    break;
	case KEY_BACKSPACE:
	    if (cur <= 0)
		break;
	    cur--;
	    /* fallthrough */
	case KEY_DC:
	    if (cur > l)
		break;
	    memmove(buf + cur, buf + cur + 1, l - cur - 1);
	    l--;
	    break;
	case KEY_ENTER:
	case '\n':
	case '\r':
	    buf[l] = 0;
	    t = 0;
	    break;
	}
    }
    return l;
}
