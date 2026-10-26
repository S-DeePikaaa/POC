/* storage.c - linked-list heads, text-file persistence and record lookups */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "library.h"

Book        *bookHead   = NULL;
Member      *memberHead = NULL;
Transaction *transHead  = NULL;

#define LINE_MAX_LEN 512
#define SEP '|'

/* ------------------------- Append to list tail ------------------------ */

void appendBook(Book *node) {
    node->next = NULL;
    if (!bookHead) { bookHead = node; return; }
    Book *p = bookHead;
    while (p->next) p = p->next;
    p->next = node;
}

void appendMember(Member *node) {
    node->next = NULL;
    if (!memberHead) { memberHead = node; return; }
    Member *p = memberHead;
    while (p->next) p = p->next;
    p->next = node;
}

void appendTransaction(Transaction *node) {
    node->next = NULL;
    if (!transHead) { transHead = node; return; }
    Transaction *p = transHead;
    while (p->next) p = p->next;
    p->next = node;
}

/* ------------------------- Text-file helpers -------------------------- */
/*
 * File formats (one record per line, fields separated by '|'):
 *   books.txt         id|title|author|category|totalCopies|availableCopies
 *   members.txt       id|name|phone|email
 *   transactions.txt  transId|bookId|memberId|issueDate|dueDate|returnDate|returned|fine
 * Dates are DD-MM-YYYY ('-' means "not returned yet").
 */

/* Writes a text field, replacing any '|' so it cannot break the format. */
static void putField(FILE *f, const char *s) {
    for (; *s; s++) fputc(*s == SEP ? '/' : *s, f);
}

/* Splits 'line' in place at each '|'. Returns the number of fields found. */
static int splitFields(char *line, char *fields[], int max) {
    int n = 0;
    char *p = line;
    while (n < max) {
        fields[n++] = p;
        p = strchr(p, SEP);
        if (!p) break;
        *p++ = '\0';
    }
    return n;
}

/* Reads the next non-empty line (newline stripped). Returns 0 at EOF. */
static int nextLine(FILE *f, char *buf, size_t size) {
    while (fgets(buf, (int)size, f)) {
        buf[strcspn(buf, "\r\n")] = '\0';
        if (!isBlank(buf)) return 1;
    }
    return 0;
}

/* ---------------------------- Saving --------------------------------- */

void saveAll(void) {
    FILE *f;
    char d1[16], d2[16], d3[16];

    if ((f = fopen(BOOK_FILE, "w"))) {
        for (Book *p = bookHead; p; p = p->next) {
            fprintf(f, "%d%c", p->id, SEP);
            putField(f, p->title);    fputc(SEP, f);
            putField(f, p->author);   fputc(SEP, f);
            putField(f, p->category); fputc(SEP, f);
            fprintf(f, "%d%c%d\n", p->totalCopies, SEP, p->availableCopies);
        }
        fclose(f);
    } else printf("Warning: could not save books.\n");

    if ((f = fopen(MEMBER_FILE, "w"))) {
        for (Member *p = memberHead; p; p = p->next) {
            fprintf(f, "%d%c", p->id, SEP);
            putField(f, p->name);  fputc(SEP, f);
            putField(f, p->phone); fputc(SEP, f);
            putField(f, p->email); fputc('\n', f);
        }
        fclose(f);
    } else printf("Warning: could not save members.\n");

    if ((f = fopen(TRANS_FILE, "w"))) {
        for (Transaction *p = transHead; p; p = p->next) {
            fmtDate(p->issueDate, d1, sizeof d1);
            fmtDate(p->expectedReturn, d2, sizeof d2);
            fmtDate(p->returnDate, d3, sizeof d3);   /* "-" if not returned */
            fprintf(f, "%d|%d|%d|%s|%s|%s|%d|%.2f\n",
                    p->transId, p->bookId, p->memberId,
                    d1, d2, d3, p->returned, p->fine);
        }
        fclose(f);
    } else printf("Warning: could not save transactions.\n");
}

/* ---------------------------- Loading -------------------------------- */

static void loadBooks(void) {
    FILE *f = fopen(BOOK_FILE, "r");
    if (!f) return;                      /* first run: no file yet */
    char line[LINE_MAX_LEN], *fld[6];
    while (nextLine(f, line, sizeof line)) {
        if (splitFields(line, fld, 6) != 6) continue;   /* skip bad line */
        Book *b = calloc(1, sizeof(Book));
        if (!b) break;
        b->id = atoi(fld[0]);
        copyStr(b->title,    fld[1], sizeof b->title);
        copyStr(b->author,   fld[2], sizeof b->author);
        copyStr(b->category, fld[3], sizeof b->category);
        b->totalCopies     = atoi(fld[4]);
        b->availableCopies = atoi(fld[5]);
        appendBook(b);
    }
    fclose(f);
}

static void loadMembers(void) {
    FILE *f = fopen(MEMBER_FILE, "r");
    if (!f) return;
    char line[LINE_MAX_LEN], *fld[4];
    while (nextLine(f, line, sizeof line)) {
        if (splitFields(line, fld, 4) != 4) continue;
        Member *m = calloc(1, sizeof(Member));
        if (!m) break;
        m->id = atoi(fld[0]);
        copyStr(m->name,  fld[1], sizeof m->name);
        copyStr(m->phone, fld[2], sizeof m->phone);
        copyStr(m->email, fld[3], sizeof m->email);
        appendMember(m);
    }
    fclose(f);
}

static void loadTransactions(void) {
    FILE *f = fopen(TRANS_FILE, "r");
    if (!f) return;
    char line[LINE_MAX_LEN], *fld[8];
    while (nextLine(f, line, sizeof line)) {
        if (splitFields(line, fld, 8) != 8) continue;
        Transaction *t = calloc(1, sizeof(Transaction));
        if (!t) break;
        t->transId  = atoi(fld[0]);
        t->bookId   = atoi(fld[1]);
        t->memberId = atoi(fld[2]);
        if (!parseDate(fld[3], &t->issueDate) ||
            !parseDate(fld[4], &t->expectedReturn)) { free(t); continue; }
        if (!parseDate(fld[5], &t->returnDate)) t->returnDate = 0;  /* "-" */
        t->returned = atoi(fld[6]);
        t->fine     = (float)atof(fld[7]);
        appendTransaction(t);
    }
    fclose(f);
}

void loadAll(void) {
    loadBooks();
    loadMembers();
    loadTransactions();
}

/* Frees every node of all three lists (call before exiting). */
void freeAll(void) {
    while (bookHead)   { Book *t = bookHead;     bookHead = t->next;   free(t); }
    while (memberHead) { Member *t = memberHead; memberHead = t->next; free(t); }
    while (transHead)  { Transaction *t = transHead; transHead = t->next; free(t); }
}

/* ------------------------------ Lookups ------------------------------ */

Book *findBook(int id) {
    for (Book *p = bookHead; p; p = p->next) if (p->id == id) return p;
    return NULL;
}

Member *findMember(int id) {
    for (Member *p = memberHead; p; p = p->next) if (p->id == id) return p;
    return NULL;
}

int memberHasOpenLoans(int memberId) {
    for (Transaction *p = transHead; p; p = p->next)
        if (p->memberId == memberId && !p->returned) return 1;
    return 0;
}

int bookHasOpenLoans(int bookId) {
    for (Transaction *p = transHead; p; p = p->next)
        if (p->bookId == bookId && !p->returned) return 1;
    return 0;
}
