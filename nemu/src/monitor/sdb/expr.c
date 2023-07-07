/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <isa.h>
#include "memory/vaddr.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

static uint64_t eval(uint64_t, uint64_t);
static uint64_t get_main_op(uint64_t, uint64_t);
static bool check_parentheses(uint64_t, uint64_t);

enum
{
	TK_NOTYPE = 256,
	TK_EQ,
	TK_NEQ,
	TK_INT,
	TK_HEX,
	TK_REG,
	TK_AND,
	TK_DEREF,
	TK_NEG,
	TK_ADDR
};

static struct rule
{
	const char *regex;
	int token_type;
} rules[] = {

	{"\\+", '+'}, // plus 是正则表达式中的特殊字符，所以特殊处理
	{"-", '-'},
	{"\\*", '*'},
	{"/", '/'},
	{"\\(", '('},
	{"\\)", ')'},
	{" +", TK_NOTYPE}, // spaces
	{"==", TK_EQ},
	{"!=", TK_NEQ},
	{"&&", TK_AND},
	{"[1-9]+", TK_INT},
	{"0[xX][0-9a-fA-F]+", TK_HEX},
	{"\\$[$]*[a-zA-Z0-9]+", TK_REG},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
	int i;
	char error_msg[128];
	int ret;

	for (i = 0; i < NR_REGEX; i++)
	{
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if (ret != 0)
		{
			regerror(ret, &re[i], error_msg, 128);
			panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token
{
	int type;
	char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0; // 指示已经被识别出的token数目

	while (e[position] != '\0')
	{
		/* Try all rules one by one. */
		for (i = 0; i < NR_REGEX; i++)
		{
			if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
			{
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
					i, rules[i].regex, position, substr_len, substr_len, substr_start);

				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				int type;
				switch (rules[i].token_type)
				{
					case TK_NOTYPE:
						break;
						
					case '*':
						type = tokens[nr_token - 1].type;
						if (nr_token == 0 || (type != '(' && type != ')' && type != TK_INT && type != TK_HEX && type != TK_REG))
							tokens[nr_token].type = TK_DEREF;
						else
							tokens[nr_token].type = '*';
						nr_token++;
						break;

					case '-':
						type = tokens[nr_token - 1].type;
						if (nr_token == 0 || (type != '(' && type != ')' && type != TK_INT && type != TK_HEX && type != TK_REG))
							tokens[nr_token].type = TK_NEG;
						else
							tokens[nr_token].type = '-';
						nr_token++;
						break;

					default:
						tokens[nr_token].type = rules[i].token_type;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						nr_token++;
						break;
				}
				break;
			}
		}
		if (i == NR_REGEX)
		{
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true;
}

word_t expr(char *e, bool *success)
{
	if (!make_token(e))
	{
		*success = false;
		return 0;
	}

	return eval(0, nr_token - 1);
}

static uint64_t eval(uint64_t p, uint64_t q)
{
	// printf("%lu %lu\n", p, q);
	if (p > q)
	{
		assert(0);
	}
	else if (p == q)
	{
		uint64_t val = 0;

		if (tokens[p].type == TK_INT)
			sscanf(tokens[p].str, "%lu", &val);
		else if (tokens[p].type == TK_HEX)
			sscanf(tokens[p].str, "%lx", &val);
		else if (tokens[p].type == TK_REG)
			val = isa_reg_str2val(tokens[p].str + 1, NULL);
			
		return val;
	}
	else if (check_parentheses(p, q) == true)
	{
		/* The expression is surrounded by a matched pair of parentheses.
		 * If that is the case, just throw away the parentheses.
		 */
		return eval(p + 1, q - 1);
	}
	else
	{
		uint64_t main_op = get_main_op(p, q);
		
		if (tokens[main_op].type == TK_DEREF)
			return vaddr_read(eval(p + 1, q), 8);
		else if (tokens[main_op].type == TK_NEG)
			return -eval(p + 1, q);

		uint64_t val1 = eval(p, main_op - 1);
		uint64_t val2 = eval(main_op + 1, q);

		switch (tokens[main_op].type)
		{
		case '+':
			return val1 + val2;
		case '-':
			return val1 - val2;
		case '*':
			return val1 * val2;
		case '/':
			return val1 / val2;
		case TK_EQ:
			return val1 == val2;
		case TK_NEQ:
			return val1 != val2;
		case TK_AND:
			return val1 && val2;
		default:
			assert(0);
		}
	}
}

static uint64_t get_main_op(uint64_t p, uint64_t q)
{
	uint64_t main_op = 0;

	if ((tokens[p].type == TK_DEREF || tokens[p].type == TK_NEG) && (p == q - 1 || check_parentheses(p + 1, q)))
		main_op = p;

	int type1, type2;
	while (p <= q)
	{
		type1 = tokens[p].type;
		switch (type1)
		{
		case '(':
			while (type1 != ')')
				p++;
			break;
		case '+':
		case '-':
		case '*':
		case '/':
		case TK_EQ:
		case TK_NEQ:
		case TK_AND:
			if (main_op == 0)
				main_op = p;
			else
			{
				type2 = tokens[main_op].type;
				if (type1 == TK_AND)
					main_op = p;
				else if ((type1 == TK_EQ || type1 == TK_NEQ) && type2 != TK_AND)
					main_op = p;
				else if ((type1 == '+' || type1 == '-') && type2 != TK_AND && type2 != TK_NEQ && type2 != TK_EQ)
					main_op = p;
				else if ((type1 == '*' || type1 == '/') && (type2 == '*' || type2 == '/'))
					main_op = p;
			}
			break;
		}
		p++;
	}
	return main_op;
}

static bool check_parentheses(uint64_t p, uint64_t q)
{
	if (tokens[p].type == '(' && tokens[q].type == ')')
	{
		char s[32];
		int sp = 0;
		int i;
		for (i = 1; i < q; ++i)
		{
			switch (tokens[i].type)
			{
				case '(':
					s[sp] = '(';
					sp++;
					break;
				case ')':
					if (sp >= 1 && s[sp - 1] == '(')
						sp--;
					else
						return false;
			}
		}
		if (sp == 0)
			return true;
	}
	return false;
}
