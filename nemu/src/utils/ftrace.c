#include <elf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Elf64_Ehdr elf_header;
Elf64_Shdr elf_sec_header_entry;
Elf64_Sym elf_symtab_entry;

struct func_info {
    uint64_t offset;
    uint32_t size;
    char *name;
    struct func_info *next;
} funcs = { .next = NULL };

struct __StackEntry{
    uint64_t dst_addr; // 目标位置函数地址
    char *dst_name;
    uint64_t addr;       // 指令所在地址
    int type;           // call 或 return
    struct __StackEntry *next;
} tfunc = { .next = NULL };

void parse_inst(int type, uint64_t addr, uint64_t dst_addr) {
    static struct __StackEntry *p = &tfunc;

    p->next = (struct __StackEntry *) malloc(sizeof(struct __StackEntry));
    p->next->type = type;
    p->next->addr = addr;
    p->next->dst_addr = dst_addr;

    struct func_info *p2 = &funcs;
    while (p2->next) {
        if (dst_addr < p2->next->offset + p2->next->size && dst_addr >= p2->next->offset) {
            p->next->dst_name = p2->next->name;
        }
        p2 = p2->next;
    }
    p = p->next;
}


void print_ftrace() {
    struct __StackEntry *p = &tfunc;
    while (p->next) {
        if (p->next->type == 1)
            printf("0x%08lx: call [%s@0x%08lx]\n", p->next->addr, p->next->dst_name, p->next->dst_addr);
        else
            printf("0x%08lx: ret [%s@0x%08lx]\n", p->next->addr, p->next->dst_name, p->next->dst_addr);
        p = p->next;
    printf("1111");

    }
    printf("1111");
}


void parse_elf(char *f) {
    FILE *fp = fopen(f, "r");
    char *strtab = NULL;
    int __attribute__ ((unused)) a;

    a = fread(&elf_header, sizeof(elf_header), 1, fp);

    if (elf_header.e_ident[0] != 0x7F ||
		elf_header.e_ident[1] != 'E' ||
		elf_header.e_ident[2] != 'L' ||
		elf_header.e_ident[3] != 'F')
	{
		printf("Not a ELF file\n");
		exit(0);
	}

    for (int i = 0; i < elf_header.e_shnum; ++i) {
        fseek(fp, elf_header.e_shoff + i * elf_header.e_shentsize, SEEK_SET);
        a = fread(&elf_sec_header_entry, elf_header.e_shentsize, 1, fp);

        if (elf_sec_header_entry.sh_type == SHT_STRTAB) {
            fseek(fp, elf_sec_header_entry.sh_offset, SEEK_SET);
            strtab = (char *) malloc(elf_sec_header_entry.sh_size);
            a = fread(strtab, elf_sec_header_entry.sh_size, 1, fp);
            break;
        }
    }

    for (int i = 0; i < elf_header.e_shnum; ++i) {
        fseek(fp, elf_header.e_shoff + i * elf_header.e_shentsize, SEEK_SET);
        a = fread(&elf_sec_header_entry, elf_header.e_shentsize, 1, fp);

        if (elf_sec_header_entry.sh_type == SHT_SYMTAB) {
            fseek(fp, elf_sec_header_entry.sh_offset, SEEK_SET);

            struct func_info *head = &funcs;
            for (int i = 0; i < elf_sec_header_entry.sh_size / elf_sec_header_entry.sh_entsize; ++i) {

                // fseek(fp, elf_sec_header_entry.sh_offset + i * elf_sec_header_entry.sh_entsize, SEEK_SET);
                // printf("%ld\n", elf_sec_header_entry.sh_entsize);
                a = fread(&elf_symtab_entry, elf_sec_header_entry.sh_entsize, 1, fp);
                if (ELF64_ST_TYPE(elf_symtab_entry.st_info) == STT_FUNC) {
                    head->next = (struct func_info *) malloc(sizeof(struct func_info));
                    head->next->offset = elf_symtab_entry.st_value;
                    head->next->size = elf_symtab_entry.st_size;
                    head->next->name = strtab + elf_symtab_entry.st_name;
                    // printf("%d %s\n", elf_symtab_entry.st_name, head->next->name);

                    head->next->next = NULL;
                    head = head->next;
                }
            }
        }
    }
}


