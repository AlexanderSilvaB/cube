#include "linkedList.h"
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct
{
    unsigned char _EI_MAG[4];
    unsigned char _EI_CLASS;
    unsigned char _EI_DATA;
    unsigned char _EI_VERSION;
    unsigned char _EI_OSABI;
    unsigned char _EI_ABIVERSION;
    unsigned char _EI_PAD[6];
    unsigned char _EI_NIDENT;
} Elf_ident;

/*
 * This is a handle that is created by elf_open and then used by every
 * other function in the elf package
 */
typedef struct
{
    bool is64;
    void *maddr;       /* Start of mapped Elf binary segment in memory */
    int mlen;          /* Length in bytes of the mapped Elf segment */
    void *ehdr;        /* Start of main Elf header (same as maddr) */
    void *symtab;      /* Start of symbol table */
    void *symtab_end;  /* End of symbol table (symtab + size) */
    char *strtab;      /* Address of string table */
    void *dsymtab;     /* Start of dynamic symbol table */
    void *dsymtab_end; /* End of dynamic symbol table (dsymtab + size) */
    char *dstrtab;     /* Address of dynamic string table */
} Elf_obj;

/*
 * Create and destroy Elf object handles
 */
Elf_obj *elf_open(char *filename);
void elf_close(Elf_obj *ep);

/*
 * Functions for manipulating static symbols
 */

/* Returns pointer to the first symbol */
Elf32_Sym *elf_firstsym32(Elf_obj *ep);
Elf64_Sym *elf_firstsym64(Elf_obj *ep);

/* Returns pointer to the next symbol, or NULL if no more symbols */
Elf32_Sym *elf_nextsym32(Elf_obj *ep, Elf32_Sym *sym);
Elf64_Sym *elf_nextsym64(Elf_obj *ep, Elf64_Sym *sym);

/* Return symbol string name */
char *elf_symname32(Elf_obj *ep, Elf32_Sym *sym);
char *elf_symname64(Elf_obj *ep, Elf64_Sym *sym);

/* True if symbol is a function */
int elf_isfunc32(Elf_obj *ep, Elf32_Sym *sym);
int elf_isfunc64(Elf_obj *ep, Elf64_Sym *sym);

/*
 * Corresponding functions for manipulating dynamic symbols
 */
Elf32_Sym *elf_firstdsym32(Elf_obj *ep);
Elf64_Sym *elf_firstdsym64(Elf_obj *ep);
Elf32_Sym *elf_nextdsym32(Elf_obj *ep, Elf32_Sym *sym);
Elf64_Sym *elf_nextdsym64(Elf_obj *ep, Elf64_Sym *sym);
char *elf_dsymname32(Elf_obj *ep, Elf32_Sym *sym);
char *elf_dsymname64(Elf_obj *ep, Elf64_Sym *sym);
int elf_isdfunc32(Elf_obj *ep, Elf32_Sym *sym);
int elf_isdfunc64(Elf_obj *ep, Elf64_Sym *sym);

/*
 * elf_open - Map a binary into the address space and extract the
 * locations of the static and dynamic symbol tables and their string
 * tables. Return this information in a Elf object file handle that will
 * be passed to all of the other elf functions.
 */
Elf_obj *elf_open(char *filename)
{
    int i, fd;
    struct stat sbuf;
    Elf_obj *ep;

    if ((ep = (Elf_obj *)malloc(sizeof(Elf_obj))) == NULL)
    {
        fprintf(stderr, "Malloc failed: %s\n", strerror(errno));
        return NULL;
    }

    /* Do some consistency checks on the binary */
    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "Can't open \"%s\": %s\n", filename, strerror(errno));
        free(ep);
        return NULL;
    }

    Elf_ident e_ident;
    read(fd, &e_ident, sizeof(Elf_ident));
    lseek(fd, 0, SEEK_SET);

    ep->is64 = e_ident._EI_CLASS == 2;

    if (fstat(fd, &sbuf) == -1)
    {
        fprintf(stderr, "Can't stat \"%s\": %s\n", filename, strerror(errno));
        close(fd);
        free(ep);
        return NULL;
    }

    if ((ep->is64 && sbuf.st_size < sizeof(Elf64_Ehdr)) || (!ep->is64 && sbuf.st_size < sizeof(Elf32_Ehdr)))
    {
        fprintf(stderr, "\"%s\" is not an ELF binary object\n", filename);
        close(fd);
        free(ep);
        return NULL;
    }

    /* It looks OK, so map the Elf binary into our address space */
    ep->mlen = sbuf.st_size;
    ep->maddr = mmap(NULL, ep->mlen, PROT_READ, MAP_SHARED, fd, 0);
    if (ep->maddr == (void *)-1)
    {
        fprintf(stderr, "Can't mmap \"%s\": %s\n", filename, strerror(errno));
        close(fd);
        free(ep);
        return NULL;
    }
    close(fd);

    /* The Elf binary begins with the Elf header */
    ep->ehdr = ep->maddr;

    /* Make sure that this is an Elf binary */
    if ((ep->is64 && strncmp(((Elf64_Ehdr *)ep->ehdr)->e_ident, ELFMAG, SELFMAG)) ||
        (!ep->is64 && strncmp(((Elf32_Ehdr *)ep->ehdr)->e_ident, ELFMAG, SELFMAG)))
    {
        fprintf(stderr, "\"%s\" is not an ELF binary object\n", filename);
        free(ep);
        return NULL;
    }

    /*
     * Find the static and dynamic symbol tables and their string
     * tables in the the mapped binary. The sh_link field in symbol
     * table section headers gives the section index of the string
     * table for that symbol table.
     */
    if (!ep->is64)
    {
        Elf32_Shdr *shdr = (Elf32_Shdr *)(ep->maddr + ((Elf32_Ehdr *)ep->ehdr)->e_shoff);
        for (i = 0; i < ((Elf32_Ehdr *)ep->ehdr)->e_shnum; i++)
        {
            if (shdr[i].sh_type == SHT_SYMTAB)
            { /* Static symbol table */
                ep->symtab = (Elf32_Sym *)(ep->maddr + shdr[i].sh_offset);
                ep->symtab_end = (Elf32_Sym *)((char *)ep->symtab + shdr[i].sh_size);
                ep->strtab = (char *)(ep->maddr + shdr[shdr[i].sh_link].sh_offset);
            }
            if (shdr[i].sh_type == SHT_DYNSYM)
            { /* Dynamic symbol table */
                ep->dsymtab = (Elf32_Sym *)(ep->maddr + shdr[i].sh_offset);
                ep->dsymtab_end = (Elf32_Sym *)((char *)ep->dsymtab + shdr[i].sh_size);
                ep->dstrtab = (char *)(ep->maddr + shdr[shdr[i].sh_link].sh_offset);
            }
        }
    }
    else
    {
        Elf64_Shdr *shdr = (Elf64_Shdr *)(ep->maddr + ((Elf64_Ehdr *)ep->ehdr)->e_shoff);
        for (i = 0; i < ((Elf64_Ehdr *)ep->ehdr)->e_shnum; i++)
        {
            if (shdr[i].sh_type == SHT_SYMTAB)
            { /* Static symbol table */
                ep->symtab = (Elf64_Sym *)(ep->maddr + shdr[i].sh_offset);
                ep->symtab_end = (Elf64_Sym *)((char *)ep->symtab + shdr[i].sh_size);
                ep->strtab = (char *)(ep->maddr + shdr[shdr[i].sh_link].sh_offset);
            }
            if (shdr[i].sh_type == SHT_DYNSYM)
            { /* Dynamic symbol table */
                ep->dsymtab = (Elf64_Sym *)(ep->maddr + shdr[i].sh_offset);
                ep->dsymtab_end = (Elf64_Sym *)((char *)ep->dsymtab + shdr[i].sh_size);
                ep->dstrtab = (char *)(ep->maddr + shdr[shdr[i].sh_link].sh_offset);
            }
        }
    }
    return ep;
}

/*
 * elf_close - Free up the resources of an  elf object
 */
void elf_close(Elf_obj *ep)
{
    if (munmap(ep->maddr, ep->mlen) < 0)
    {
        perror("munmap");
        return;
    }
    free(ep);
}

/*
 * elf_symname - Return ASCII name of a static symbol
 */
char *elf_symname32(Elf_obj *ep, Elf32_Sym *sym)
{
    return &ep->strtab[sym->st_name];
}
char *elf_symname64(Elf_obj *ep, Elf64_Sym *sym)
{
    return &ep->strtab[sym->st_name];
}

/*
 * elf_dsymname - Return ASCII name of a dynamic symbol
 */
char *elf_dsymname32(Elf_obj *ep, Elf32_Sym *sym)
{
    return &ep->dstrtab[sym->st_name];
}
char *elf_dsymname64(Elf_obj *ep, Elf64_Sym *sym)
{
    return &ep->dstrtab[sym->st_name];
}

/*
 * elf_firstsym - Return ptr to first symbol in static symbol table
 */
Elf32_Sym *elf_firstsym32(Elf_obj *ep)
{
    return ep->symtab;
}
Elf64_Sym *elf_firstsym64(Elf_obj *ep)
{
    return ep->symtab;
}

/*
 * elf_nextsym - Return ptr to next symbol in static symbol table,
 * or NULL if no more symbols.
 */
Elf32_Sym *elf_nextsym32(Elf_obj *ep, Elf32_Sym *sym)
{
    sym++;
    if (sym < (Elf32_Sym *)ep->symtab_end)
        return sym;
    else
        return NULL;
}
Elf64_Sym *elf_nextsym64(Elf_obj *ep, Elf64_Sym *sym)
{
    sym++;
    if (sym < (Elf64_Sym *)ep->symtab_end)
        return sym;
    else
        return NULL;
}

/*
 * elf_firstdsym - Return ptr to first symbol in dynamic symbol table
 */
Elf32_Sym *elf_firstdsym32(Elf_obj *ep)
{
    return ep->dsymtab;
}
Elf64_Sym *elf_firstdsym64(Elf_obj *ep)
{
    return ep->dsymtab;
}

/*
 * elf_nextdsym - Return ptr to next symbol in dynamic symbol table,
 * of NULL if no more symbols.
 */
Elf32_Sym *elf_nextdsym32(Elf_obj *ep, Elf32_Sym *sym)
{
    sym++;
    if (sym < (Elf32_Sym *)ep->dsymtab_end)
        return sym;
    else
        return NULL;
}
Elf64_Sym *elf_nextdsym64(Elf_obj *ep, Elf64_Sym *sym)
{
    sym++;
    if (sym < (Elf64_Sym *)ep->dsymtab_end)
        return sym;
    else
        return NULL;
}

/*
 * elf_isfunc - Return true if symbol is a static function
 */
int elf_isfunc32(Elf_obj *ep, Elf32_Sym *sym)
{
    return ((ELF32_ST_TYPE(sym->st_info) == STT_FUNC) && (sym->st_shndx != SHT_NULL));
}
int elf_isfunc64(Elf_obj *ep, Elf64_Sym *sym)
{
    return ((ELF64_ST_TYPE(sym->st_info) == STT_FUNC) && (sym->st_shndx != SHT_NULL));
}

/*
 * elf_isdfunc - Return true if symbol is a dynamic function
 */
int elf_isdfunc32(Elf_obj *ep, Elf32_Sym *sym)
{
    return ((ELF32_ST_TYPE(sym->st_info) == STT_FUNC));
}
int elf_isdfunc64(Elf_obj *ep, Elf64_Sym *sym)
{
    return ((ELF64_ST_TYPE(sym->st_info) == STT_FUNC));
}

linked_list *list_symbols(const char *path)
{
    Elf_obj *elfp = elf_open((char *)path);
    void *sp;

    if (!elfp)
    {
        return NULL;
    }

    linked_list *list = linked_list_create();
    char *name;
    bool is_func;

    if (elfp->is64)
        sp = elf_firstdsym64(elfp);
    else
        sp = elf_firstdsym32(elfp);
    while (sp != NULL)
    {
        if (elfp->is64)
        {
            name = elf_dsymname64(elfp, sp);
            is_func = elf_isdfunc64(elfp, sp);
            sp = elf_nextdsym64(elfp, sp);
        }
        else
        {
            name = elf_dsymname32(elfp, sp);
            is_func = elf_isdfunc32(elfp, sp);
            sp = elf_nextdsym32(elfp, sp);
        }

        if (strlen(name) > 0)
        {
            symbol_t *sym = (symbol_t *)malloc(sizeof(symbol_t));
            sym->is_func = is_func;
            sym->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
            strcpy(sym->name, name);
            linked_list_add(list, sym);
        }
    }

    elf_close(elfp);

    return list;
}