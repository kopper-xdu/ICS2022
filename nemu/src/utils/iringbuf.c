typedef struct iringbuf {
	char buf[10][128];
	int size;
	int beg;
} iringbuf;

static iringbuf buf = { .size = 0, .beg = 0};

char *strcpy(char *dest, const char *src);
int puts(const char *s);

void write_to_iringbuf(char *s) {
    if (buf.size < 10) {
        strcpy(buf.buf[buf.size++], s);
    } else {
        if (buf.beg == 10) {
            buf.beg = 0;
        }
        strcpy(buf.buf[buf.beg++], s);
    }
}
int puts(const char *s);
void print_iringbuf() {
    if (buf.beg == 10)
        buf.beg = 0;
    int j = 0;
    for (int i = buf.beg; j < buf.size; ++i, ++j) {
        if (i == 10)
            i = 0;
        puts(buf.buf[i]);
    }
}
