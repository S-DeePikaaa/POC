/* book.c - book management, search and availability (linked list) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "library.h"

static void printBookHeader(void) {
    printf("\n%-6s %-30s %-20s %-15s %-6s %-9s\n",
           "ID", "Title", "Author", "Category", "Total", "Available");
    printf("------------------------------------------------------------------------------------\n");
}

static void printBook(const Book *b) {
    printf("%-6d %-30.30s %-20.20s %-15.15s %-6d %-9d\n",
           b->id, b->title, b->author, b->category,
           b->totalCopies, b->availableCopies);
}

void addBook(void) {
    Book b;
    int id, copies;

    if (!readInt("Enter Book ID: ", &id) || id <= 0) {
        printf("Invalid Book ID. It must be a positive number.\n"); return;
    }
    if (findBook(id)) {
        printf("Error: Book ID %d already exists.\n", id); return;
    }
    b.id = id;

    readLine("Enter Title: ", b.title, sizeof b.title);
    if (isBlank(b.title)) { printf("Title cannot be empty.\n"); return; }
    readLine("Enter Author: ", b.author, sizeof b.author);
    if (isBlank(b.author)) { printf("Author cannot be empty.\n"); return; }
    readLine("Enter Category: ", b.category, sizeof b.category);
    if (isBlank(b.category)) { printf("Category cannot be empty.\n"); return; }

    if (!readInt("Enter number of copies: ", &copies) || copies <= 0) {
        printf("Invalid quantity. It must be a positive number.\n"); return;
    }
    b.totalCopies = b.availableCopies = copies;

    Book *node = malloc(sizeof(Book));
    if (!node) { printf("Out of memory.\n"); return; }
    *node = b;
    appendBook(node);
    saveAll();
    printf("Book added successfully.\n");
}

void viewBooks(void) {
    if (!bookHead) { printf("No books in the library.\n"); return; }
    printBookHeader();
    for (Book *p = bookHead; p; p = p->next) printBook(p);
}

void updateBook(void) {
    int id;
    if (!readInt("Enter Book ID to update: ", &id)) {
        printf("Invalid Book ID.\n"); return;
    }
    Book *b = findBook(id);
    if (!b) { printf("Book not found.\n"); return; }

    char buf[100];
    printf("(Press Enter to keep the current value)\n");

    printf("Title [%s]\n", b->title);
    readLine("New Title: ", buf, sizeof buf);
    if (!isBlank(buf)) copyStr(b->title, buf, sizeof b->title);

    printf("Author [%s]\n", b->author);
    readLine("New Author: ", buf, sizeof buf);
    if (!isBlank(buf)) copyStr(b->author, buf, sizeof b->author);

    printf("Category [%s]\n", b->category);
    readLine("New Category: ", buf, sizeof buf);
    if (!isBlank(buf)) copyStr(b->category, buf, sizeof b->category);

    printf("Total copies [%d] (issued: %d)\n",
           b->totalCopies, b->totalCopies - b->availableCopies);
    readLine("New total copies: ", buf, sizeof buf);
    if (!isBlank(buf)) {
        char *end;
        long n = strtol(buf, &end, 10);
        int issued = b->totalCopies - b->availableCopies;
        if (*end != '\0' || n <= 0) {
            printf("Invalid quantity; copies not changed.\n");
        } else if (n < issued) {
            printf("Cannot set copies below the %d currently issued; not changed.\n",
                   issued);
        } else {
            b->totalCopies = (int)n;
            b->availableCopies = (int)n - issued;
        }
    }
    saveAll();
    printf("Book updated successfully.\n");
}

void deleteBook(void) {
    int id;
    if (!readInt("Enter Book ID to delete: ", &id)) {
        printf("Invalid Book ID.\n"); return;
    }
    if (!findBook(id)) { printf("Book not found.\n"); return; }
    if (bookHasOpenLoans(id)) {
        printf("Cannot delete: copies of this book are currently issued.\n");
        return;
    }

    /* unlink the node, keeping track of the previous one */
    Book *prev = NULL, *cur = bookHead;
    while (cur && cur->id != id) { prev = cur; cur = cur->next; }
    if (prev) prev->next = cur->next;
    else      bookHead = cur->next;
    free(cur);

    saveAll();
    printf("Book deleted successfully.\n");
}

/* ------------------------ Search & availability ----------------------- */

void searchBooks(void) {
    printf("\nSearch by: 1.Book ID  2.Title  3.Author  4.Category\n");
    int choice;
    if (!readInt("Choice: ", &choice) || choice < 1 || choice > 4) {
        printf("Invalid choice.\n"); return;
    }

    int found = 0, idSearch = 0;
    char key[100] = "";

    if (choice == 1) {
        if (!readInt("Enter Book ID: ", &idSearch)) {
            printf("Invalid Book ID.\n"); return;
        }
    } else {
        readLine("Enter search text: ", key, sizeof key);
        if (isBlank(key)) { printf("Search text cannot be empty.\n"); return; }
    }

    for (Book *p = bookHead; p; p = p->next) {
        int match = 0;
        switch (choice) {
            case 1: match = (p->id == idSearch); break;
            case 2: match = containsIgnoreCase(p->title, key); break;
            case 3: match = containsIgnoreCase(p->author, key); break;
            case 4: match = containsIgnoreCase(p->category, key); break;
        }
        if (match) {
            if (!found) printBookHeader();
            printBook(p);
            printf("        -> %s\n", p->availableCopies > 0
                   ? "Available for issue" : "Not available (all copies issued)");
            found = 1;
        }
    }
    if (!found) printf("No matching books found.\n");
}

void checkAvailability(void) {
    int id;
    if (!readInt("Enter Book ID: ", &id)) { printf("Invalid Book ID.\n"); return; }
    Book *b = findBook(id);
    if (!b) { printf("Book not found.\n"); return; }
    printf("'%s' : %d of %d copies available -> %s\n",
           b->title, b->availableCopies, b->totalCopies,
           b->availableCopies > 0 ? "AVAILABLE" : "NOT AVAILABLE");
}

/* -------------------------------- Menu -------------------------------- */

void bookMenu(void) {
    int c;
    do {
        printf("\n--- BOOK MANAGEMENT ---\n"
               "1. Add Book\n2. View All Books\n3. Update Book\n"
               "4. Delete Book\n0. Back\n");
        if (!readInt("Choice: ", &c)) { printf("Invalid input.\n"); c = -1; continue; }
        switch (c) {
            case 1: addBook(); break;
            case 2: viewBooks(); break;
            case 3: updateBook(); break;
            case 4: deleteBook(); break;
            case 0: break;
            default: printf("Invalid choice.\n");
        }
    } while (c != 0);
}
