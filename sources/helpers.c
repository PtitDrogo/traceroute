#include "../includes/traceroute.h"

size_t ft_strlcpy(char *dst, const char *src, size_t dsize) {
    const char *osrc = src;
    size_t nleft = dsize;

    if (nleft != 0) {
        while (--nleft != 0) {
            if ((*dst++ = *src++) == '\0')
                break;
        }
    }

    if (nleft == 0) {
        if (dsize != 0)
            *dst = '\0';
        while (*src++)
            ;
    }

    return (src - osrc - 1);
}