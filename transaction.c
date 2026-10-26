/* transaction.c - issue, return, transaction records, fines (linked list) */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "library.h"

static int nextTransId(void) {
    int max = 0;
    for (Transaction *p = transHead; p; p = p->next)
        if (p->transId > max) max = p->transId;
    return max + 1;
}

void issueBook(void) {
    int bookId, memberId;

    if (!readInt("Enter Book ID: ", &bookId)) { printf("Invalid Book ID.\n"); return; }
    Book *book = findBook(bookId);
    if (!book) { printf("Book not found.\n"); return; }

    if (!readInt("Enter Member ID: ", &memberId)) { printf("Invalid Member ID.\n"); return; }
    if (!findMember(memberId)) {
        printf("Member not registered. Register the member first.\n"); return;
    }

    if (book->availableCopies <= 0) {
        printf("Sorry, '%s' is currently unavailable.\n", book->title); return;
    }

    /* prevent the same member from borrowing the same title twice */
    for (Transaction *p = transHead; p; p = p->next)
        if (!p->returned && p->bookId == bookId && p->memberId == memberId) {
            printf("This member already has this book issued.\n"); return;
        }

    time_t issue;
    if (!readDate("Issue date (DD-MM-YYYY, Enter = today): ", &issue)) {
        printf("Invalid date.\n"); return;
    }

    Transaction *t = malloc(sizeof(Transaction));
    if (!t) { printf("Out of memory.\n"); return; }
    t->transId = nextTransId();
    t->bookId = bookId;
    t->memberId = memberId;
    t->issueDate = issue;
    t->expectedReturn = issue + (time_t)LOAN_DAYS * 86400;
    /* normalise to midnight in case of a DST shift */
    struct tm tmx = *localtime(&t->expectedReturn);
    tmx.tm_hour = 0; tmx.tm_min = 0; tmx.tm_sec = 0; tmx.tm_isdst = -1;
    t->expectedReturn = mktime(&tmx);
    t->returnDate = 0;
    t->returned = 0;
    t->fine = 0.0f;

    appendTransaction(t);
    book->availableCopies--;
    saveAll();

    char d1[16], d2[16];
    fmtDate(t->issueDate, d1, sizeof d1);
    fmtDate(t->expectedReturn, d2, sizeof d2);
    printf("Book issued. Transaction ID: %d\n", t->transId);
    printf("Issue date: %s | Expected return: %s | Copies left: %d\n",
           d1, d2, book->availableCopies);
}

void returnBook(void) {
    int bookId, memberId;

    if (!readInt("Enter Book ID: ", &bookId)) { printf("Invalid Book ID.\n"); return; }
    if (!readInt("Enter Member ID: ", &memberId)) { printf("Invalid Member ID.\n"); return; }
    Book *book = findBook(bookId);
    if (!book) { printf("Book not found.\n"); return; }
    if (!findMember(memberId)) { printf("Member not found.\n"); return; }

    Transaction *t = NULL;
    for (Transaction *p = transHead; p; p = p->next)
        if (!p->returned && p->bookId == bookId && p->memberId == memberId) {
            t = p; break;
        }
    if (!t) {
        printf("No active issue record for this book and member.\n"); return;
    }

    time_t ret;
    if (!readDate("Return date (DD-MM-YYYY, Enter = today): ", &ret)) {
        printf("Invalid date.\n"); return;
    }
    if (ret < t->issueDate) {
        printf("Return date cannot be before the issue date.\n"); return;
    }

    t->returnDate = ret;
    t->returned = 1;

    int overdue = daysBetween(t->expectedReturn, ret);
    if (overdue > 0) {
        t->fine = overdue * FINE_PER_DAY;
        printf("Book returned LATE by %d day(s). Fine: Rs. %.2f\n", overdue, t->fine);
    } else {
        t->fine = 0.0f;
        printf("Book returned on time. No fine.\n");
    }

    book->availableCopies++;   /* inventory updated automatically */
    saveAll();
    printf("Inventory updated. Copies available: %d\n", book->availableCopies);
}

void viewTransactions(void) {
    if (!transHead) { printf("No transactions recorded.\n"); return; }
    printf("\n%-6s %-6s %-8s %-12s %-12s %-12s %-9s %-8s\n",
           "TID", "Book", "Member", "Issued", "Due", "Returned", "Status", "Fine");
    printf("--------------------------------------------------------------------------------\n");
    for (Transaction *p = transHead; p; p = p->next) {
        char a[16], b[16], c[16];
        fmtDate(p->issueDate, a, sizeof a);
        fmtDate(p->expectedReturn, b, sizeof b);
        fmtDate(p->returnDate, c, sizeof c);
        printf("%-6d %-6d %-8d %-12s %-12s %-12s %-9s %-8.2f\n",
               p->transId, p->bookId, p->memberId, a, b, c,
               p->returned ? "Returned" : "Issued", p->fine);
    }
}

void memberFineDetails(void) {
    int memberId;
    if (!readInt("Enter Member ID: ", &memberId)) { printf("Invalid Member ID.\n"); return; }
    Member *m = findMember(memberId);
    if (!m) { printf("Member not found.\n"); return; }

    float total = 0.0f;
    int any = 0;
    time_t now = today();

    printf("\nFine details for %s (ID %d)\n", m->name, memberId);
    printf("%-6s %-6s %-12s %-12s %-12s %-10s\n",
           "TID", "Book", "Issued", "Due", "Returned", "Fine");
    printf("--------------------------------------------------------------\n");
    for (Transaction *p = transHead; p; p = p->next) {
        if (p->memberId != memberId) continue;
        float fine = p->fine;
        char a[16], b[16], c[16];
        fmtDate(p->issueDate, a, sizeof a);
        fmtDate(p->expectedReturn, b, sizeof b);
        fmtDate(p->returnDate, c, sizeof c);
        /* for unreturned books show the fine accrued so far */
        if (!p->returned) {
            int od = daysBetween(p->expectedReturn, now);
            fine = od > 0 ? od * FINE_PER_DAY : 0.0f;
        }
        printf("%-6d %-6d %-12s %-12s %-12s %-10.2f%s\n",
               p->transId, p->bookId, a, b, c, fine,
               p->returned ? "" : " (accruing)");
        total += fine;
        any = 1;
    }
    if (!any) printf("No transactions for this member.\n");
    else printf("\nTotal fine (incl. accruing): Rs. %.2f\n", total);
}
