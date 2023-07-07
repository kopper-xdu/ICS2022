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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint
{
	int NO;
	struct watchpoint *next;
	char exp[128];
	uint64_t val;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

static void free_wp_(WP *wp, WP *pre);

void init_wp_pool()
{
	int i;
	for (i = 0; i < NR_WP; i++)
	{
		wp_pool[i].NO = i;
		wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
	}

	head = NULL;
	free_ = wp_pool;
}

void print_wp() {
	WP *wp = head;
	while (wp) {
		printf("watch point NO.%d: %lu, 0x%lx\n", wp->NO, wp->val, wp->val);
		wp = wp->next;
	}
}

bool wp_difftest() {
	WP *wp = head;
	bool flag = false;

	while (wp) {
		uint64_t new_val = expr(wp->exp, NULL);
		if (new_val != wp->val) {
			flag = true;
			printf("watch point NO.%d changed: %lu, 0x%lx -> %lu, 0x%lx\n", wp->NO, wp->val, wp->val, new_val, new_val);
			wp->val = new_val;
		}
		wp = wp->next;
	}
	return flag;
}

void new_wp(int NO, char *exp) {
	if (free_ == NULL)
		assert(0);

	WP *new = free_;
	free_ = free_->next;
	new->next = head;
	head = new;
	// printf("%lx\n", (uint64_t) free_);
	free_ = free_->next;
	// printf("%lx\n", (uint64_t) free_);

	new->NO = NO;
	strcpy(new->exp, exp);
	new->val = expr(exp, NULL);
}

void free_wp(int NO) {
	if (head && head->NO == NO) {
		free_wp_(head, NULL);
	}
	WP *p = head->next;
	WP *pre = head;
	while (p) {
		if (p->NO == NO) {
			free_wp_(p, pre);
		}
		p = p->next;
		pre = pre->next;
	}
}

void free_wp_(WP *wp, WP *pre) {
	if (pre) {
		pre->next = wp->next;
	}
	wp->next = free_;
	free_ = wp;
}


