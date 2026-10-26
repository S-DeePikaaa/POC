/*
 * library.h - shared types, constants, globals and function prototypes
 *             for the Library Management System (linked-list version).
 */
#ifndef LIBRARY_H
#define LIBRARY_H

#include <stddef.h>
#include <time.h>

/* ---------------------------- Configuration ---------------------------- */
#define LOAN_DAYS     14      /* days allowed before a book is overdue */
#define FINE_PER_DAY  2.0f    /* fine charged per overdue day (Rs.)    */

/* plain text data files: one record per line, fields separated by '|' */
#define BOOK_FILE     "books.txt"
#define MEMBER_FILE   "members.txt"
#define TRANS_FILE    "transactions.txt"

/* ------------------------------- Types --------------------------------- */
/* Each record is a node of a singly linked list. */
typedef struct Book {
    int  id;
    char title[100];
    char author[60];
    char category[40];
    int  totalCopies;
    int  availableCopies;
    struct Book *next;
} Book;

typedef struct Member {
    int  id;
    char name[60];
    char phone[20];
    char email[60];
    struct Member *next;
} Member;

typedef struct Transaction {
    int    transId;
    int    bookId;
    int    memberId;
    time_t issueDate;
    time_t expectedReturn;
    time_t returnDate;   /* 0 if not yet returned */
    int    returned;     /* 0 = issued, 1 = returned */
    float  fine;
    struct Transaction *next;
} Transaction;

/* ------------------------------ Globals -------------------------------- */
/* heads of the three linked lists, defined in storage.c */
extern Book        *bookHead;
extern Member      *memberHead;
extern Transaction *transHead;

/* ------------------------ utils.c : helpers ---------------------------- */
void   readLine(const char *prompt, char *buf, size_t size);
int    readInt(const char *prompt, int *out);
void   copyStr(char *dst, const char *src, size_t size);
int    isBlank(const char *s);
int    containsIgnoreCase(const char *text, const char *pat);
int    validPhone(const char *p);
int    validEmail(const char *e);
time_t today(void);
int    parseDate(const char *s, time_t *out);
int    readDate(const char *prompt, time_t *out);
void   fmtDate(time_t t, char *buf, size_t size);
int    daysBetween(time_t from, time_t to);

/* --------------------- storage.c : files & lookups --------------------- */
void         saveAll(void);
void         loadAll(void);
void         freeAll(void);
Book        *findBook(int id);
Member      *findMember(int id);
int          memberHasOpenLoans(int memberId);
int          bookHasOpenLoans(int bookId);
void         appendBook(Book *node);
void         appendMember(Member *node);
void         appendTransaction(Transaction *node);

/* ------------------------ book.c : books ------------------------------- */
void addBook(void);
void viewBooks(void);
void updateBook(void);
void deleteBook(void);
void searchBooks(void);
void checkAvailability(void);
void bookMenu(void);

/* ----------------------- member.c : members ---------------------------- */
void registerMember(void);
void viewMembers(void);
void searchMember(void);
void updateMember(void);
void deleteMember(void);
void memberMenu(void);

/* ------------------ transaction.c : issue/return/fine ------------------ */
void issueBook(void);
void returnBook(void);
void viewTransactions(void);
void memberFineDetails(void);

#endif /* LIBRARY_H */
