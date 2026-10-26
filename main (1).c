/* main.c - entry point and main menu */
#include <stdio.h>
#include "library.h"

int main(void) {
    int c;
    loadAll();
    printf("=========================================\n");
    printf("      LIBRARY MANAGEMENT SYSTEM\n");
    printf("=========================================\n");
    printf("Loan period: %d days | Fine: Rs. %.2f/day\n", LOAN_DAYS, FINE_PER_DAY);

    do {
        printf("\n========== MAIN MENU ==========\n"
               "1. Book Management\n"
               "2. Member Management\n"
               "3. Search Books\n"
               "4. Check Book Availability\n"
               "5. Issue Book\n"
               "6. Return Book\n"
               "7. View Transactions\n"
               "8. Member Fine Details\n"
               "0. Exit\n");
        if (!readInt("Enter choice: ", &c)) { printf("Invalid input.\n"); c = -1; continue; }
        switch (c) {
            case 1: bookMenu(); break;
            case 2: memberMenu(); break;
            case 3: searchBooks(); break;
            case 4: checkAvailability(); break;
            case 5: issueBook(); break;
            case 6: returnBook(); break;
            case 7: viewTransactions(); break;
            case 8: memberFineDetails(); break;
            case 0: saveAll(); printf("Data saved. Goodbye!\n"); break;
            default: printf("Invalid choice. Try again.\n");
        }
    } while (c != 0);
    freeAll();   /* release all linked-list nodes */
    return 0;
}
