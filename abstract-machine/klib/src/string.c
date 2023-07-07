#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s)
{
	size_t cnt = 0;
	for (; *s; ++s)
		++cnt;
	return cnt;
}

size_t strnlen(const char * s, size_t count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

char *strcpy(char *dst, const char *src)
{
	char *d = dst;
	while (*src) {
		*d++ = *src++;
	}
	*d = *src;
	return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
	char *d = dst;
	size_t slen = strlen(src);
	if (slen < n) {
		while (*src) {
			*d++ = *src++;
		}
		memset(dst + slen, 0, n - slen);
		return dst;
	} else {
		while (n--)
			*d++ = *src++;
	}
	return dst;
}

char *strcat(char *dst, const char *src)
{
	strcpy(dst + strlen(dst), src);
	return dst;

}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2) {
		if (*s1 == '\0')
			return 0;
		s1++;
		s2++;
	}
	return *(unsigned char *) s1 - *(unsigned char *) s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	while (n && *(unsigned char *) s1 == *(unsigned char *) s2) {
		if (*s1 == '\0')
			return 0;
		s1++;
		s2++;
		n--;
	}
	if (!n)
		return 0;
	return *(unsigned char *) s1 - *(unsigned char *) s2;
}

void *memset(void *s, int c, size_t n)
{
	char *s_ = (char *) s;
	for (int i = 0; i < n; ++i) {
		*s_++ = (unsigned char) c;
	}
	return s;
}

void *memmove(void *dst, const void *src, size_t n)
{
	char buf[n];
	memcpy(buf, src, n);
	memcpy(dst, buf, n);
	return dst;
}

void *memcpy(void *out, const void *in, size_t n)
{
	char *out_ = (char *) out;
	for (int i = 0; i < n; ++i) {
		*out_++ = *(char *)in++;
	}
	return (void *) ((char *) out + n);
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	for (int i = 0; i < n; ++i) {
		unsigned char c1 = *(unsigned char *) (s1 + i);
		unsigned char c2 = *(unsigned char *) (s2 + i);

		if (c1 > c2) {
			return 1;
		} else if (c1 < c2) {
			return -1;
		}
	}
	return 0;
}

#endif
