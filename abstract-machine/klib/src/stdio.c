#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// FLAGS
#define ZEROPAD	1		/* pad with zero 填补0*/
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus 显示+*/
#define SPACE	8		/* space if plus 加上空格*/
#define LEFT	16		/* left justified 左对齐*/
#define SPECIAL	32		/* 0x /0*/
#define LARGE	64	

//进制之间的相应转换
# define do_div(n,base) ({						\
	unsigned int __base = (base);					\
	unsigned int __rem;	\
	\
	if (((n) >> 32) == 0) {						\
		__rem = (unsigned int)(n) % __base;			\
		(n) = (unsigned int)(n) / __base;			\
	} else								\
		__rem = __div64_32(&(n), __base);			\
	__rem;								\
 })


static char * number(char * str, unsigned long long num, int base, int size, int precision, int type);
static int skip_atoi(const char **s);
uint32_t __attribute__((weak)) __div64_32(unsigned long long *n, uint32_t base);


int printf(const char *fmt, ...)
{
	char buf[10000];
	va_list args;
  	int i;
  	va_start (args, fmt);
  	i = vsprintf(buf, fmt, args);
	va_end (args);

	for (int k = 0; k < i; k++)
		putch(buf[k]);
	return i;
}

int sprintf(char *out, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i=vsprintf(out, fmt, args);
	va_end(args);
	return i;
}

void check(bool);
int vsprintf(char *buf, const char *fmt, va_list args)
{
	// %[flag][width][.precision]type
	int len;
	unsigned long long num;
	int i, base;
	char * str;
	const char *s;

	int flags;		/* 用在number()函数的标志 */
	
	int field_width;	/* 输出字段的宽度 */
	//精度；用在浮点数时表示输出小数点后几位；用在字符串时表示输出字符个数 
    int precision;		
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */
	                        /* 'z' support added 23/7/1999 S.H.    */
				/* 'z' changed to 'Z' --davidm 1/25/99 */
    
    
	for (str=buf ; *fmt ; ++fmt) 
    {
		if (*fmt != '%') 
        {
			*str++ = *fmt;
			continue;
		}
        
		/* process flags */
		flags = 0;
		repeat:
			++fmt;
			switch (*fmt)
            {		
				case '-': flags |= LEFT; goto repeat;
				case '+': flags |= PLUS; goto repeat;
				case ' ': flags |= SPACE; goto repeat;
				case '#': flags |= SPECIAL; goto repeat;
				case '0': flags |= ZEROPAD; goto repeat;
			}

		field_width = -1;
		if ('0' <= *fmt && *fmt <= '9')
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*')
        {             
			++fmt;
			field_width = va_arg(args, int);
            
			if (field_width < 0) //手动输入负数，左对齐，例如printf("%*d",-2,3);
            {	
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		precision = -1;
		if (*fmt == '.') 
        {
			++fmt;	
			if ('0' <= *fmt && *fmt <= '9')
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') //可变精度，printf("%.*f",2,3.1415);-->3.14
            {
				++fmt;
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}
	
		//获取转换修饰符,即%hd、%ld、%lld、%Lf...中的h、l、L、Z (ll用q代替)
		qualifier = -1;
		if (*fmt == 'l' && *(fmt + 1) == 'l') 
        {
			qualifier = 'q';
			fmt += 2;
		} else if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z') 
        {
			qualifier = *fmt;
			++fmt;
		}
	
		
		base = 10;
		switch (*fmt) 
        {
		case 'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
					
			*str++ = (unsigned char) va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			continue;
	       
		case 's':
			s = va_arg(args, char *);
			if (!s)
				s = "<NULL>";

			len = strnlen(s, precision);
	
			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			continue;
	
		case 'p':
			if (field_width == -1) //如果没有设置字段宽度
            { 
                /*字宽为8或16(根据系统而定)，因为2个位表示一个直接；
                例如32位系统指针大小位4字节，oxFF FF FF FF,需要8个字宽才能存储*/
				field_width = 2*sizeof(void *);
				flags |= ZEROPAD;   //flags = 1;会在前面补0
			}
	        //转为16进制并存进缓冲区中
			str = number(str,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			continue;
	
		//buf为1024字节空间的输出缓冲区（静态char数组）
		case 'n':
			if (qualifier == 'l') {
				long * ip = va_arg(args, long *);
				*ip = (str - buf);//获取输出缓冲数组中的个数
			} else if (qualifier == 'Z') {
				size_t * ip = va_arg(args, size_t *);
				*ip = (str - buf);
			} else {
				int * ip = va_arg(args, int *);
				*ip = (str - buf);
			}
			continue;
	
		case '%':
			*str++ = '%';
			continue;
	
		/* integer number formats - set up the flags and "break" */
		case 'o':
			base = 8;
			break;
	
		case 'X':
			flags |= LARGE;//小写转大写
		case 'x':  //十六进制
			base = 16;
			break;
	
		case 'd':	//十进制
		case 'i':
			flags |= SIGN;
		case 'u':	//无符号
			break;
	
		default:
			*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			continue;
		}
		if (qualifier == 'l') 
        {
			num = va_arg(args, unsigned long);
			if (flags & SIGN)
				num = (signed long) num;
		} else if (qualifier == 'q') 
        {
			num = va_arg(args, unsigned long long);
			if (flags & SIGN)
				num = (signed long long) num;
		} else if (qualifier == 'Z') 
        {
			num = va_arg(args, size_t);
		} else if (qualifier == 'h') 
        {
            //输出短整型时是先以整数来获取参数，再转为短整型输出，保证获取过程中精度不丢失
			num = (unsigned short) va_arg(args, int);
			if (flags & SIGN)
				num = (signed short) num;
		} else 
        {
            //没有特殊标志的格式符，一律先以无符号整型获取，再转为有符号整型
			num = va_arg(args, unsigned int);
			if (flags & SIGN)
				num = (signed int) num;
		}
        //转换为对应的个数再存到缓冲区中
		str = number(str, num, base, field_width, precision, flags);
	}
	*str = '\0';//最后以'\0'结束
	return str-buf;
}


int snprintf(char *out, size_t n, const char *fmt, ...)
{
	panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
	panic("Not implemented");
}

static int skip_atoi(const char **s)
{
	int i, c;

	for (i = 0; '0' <= (c = **s) && c <= '9'; ++*s)
		i = i*10 + c - '0';
	return i;
}

//以特定的进制格式化输出字符
static char * number(char * str, unsigned long long num, int base, int size, int precision, int type)
{
	char c,sign,tmp[66];
	const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
	int i;

	if (type & LARGE)//输出大写字符，例如十六进制0XFF112233AA
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (type & LEFT)//如果有'-'，如果出现了左对齐，就取消前面补0
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ';//如果标志符有0则补0，否则补空格；例如%02d
	sign = 0;//符号
	
	if (type & SIGN) //有符号与无符号的转换
    {
		if ((signed long long)num < 0) 
        {
			sign = '-';
			num = - (signed long long)num;//取正值
			size--;//字段宽度减1
		} else if (type & PLUS) //显示+
        {
			sign = '+';
			size--;
		} else if (type & SPACE)//填补空格
        {
			sign = ' ';
			size--;
		}
	}
	
    //处理十六进制字宽问题
	if (type & SPECIAL) //十六进制显示
    {
		if (base == 16)
			size -= 2;//0x
		else if (base == 8)
			size--;//0
	}
	i = 0;
	if (num == 0)//如果参数为0，则记录字符0
		tmp[i++]='0'; //tmp中的内容会放到缓冲区中
	else while (num != 0) //循环,num /= base
    {
		tmp[i++] = digits[num % base];    // TODO
		num /= base;
	}
    //地址长度大于精度，直接按地址长度输出，如果精度大于地址位数，先补空格
    //例如：printf("%18p\n",&a);-->空格空格00000000FAF27284
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type&(ZEROPAD+LEFT)))//没有'-'和补0,直接补空格
		while(size-->0)
			*str++ = ' ';
	if (sign)//如果有符号，输出符号，符号包括：'-','+','',0
		*str++ = sign;
    
	if (type & SPECIAL) //输出8进制或16进制符号0或0x
    {
		if (base==8)
			*str++ = '0';
		else if (base==16) 
        {
			*str++ = '0';
			*str++ = digits[33];//x或X
		}
	}
    
	if (!(type & LEFT))//没有-
		while (size-- > 0)
			*str++ = c;//c为0或空格
	while (i < precision--)//i为转换后存在tmp中字符的个数
		*str++ = '0';
	while (i-- > 0)
		*str++ = tmp[i];//tmp中存储着转换了的参数
	while (size-- > 0)
		*str++ = ' ';
	return str;
}

uint32_t __attribute__((weak)) __div64_32(unsigned long long *n, uint32_t base)
{
	uint64_t rem = *n;
	uint64_t b = base;
	uint64_t res, d = 1;
	uint32_t high = rem >> 32;

	/* Reduce the thing a bit first */
	res = 0;
	if (high >= base) {
		high /= base;
		res = (uint64_t) high << 32;
		rem -= (uint64_t) (high*base) << 32;
	}

	while ((int64_t)b > 0 && b < rem) {
		b = b+b;
		d = d+d;
	}

	do {
		if (rem >= b) {
			rem -= b;
			res += d;
		}
		b >>= 1;
		d >>= 1;
	} while (d);

	*n = res;
	return rem;
}

#endif