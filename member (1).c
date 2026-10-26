/* member.c - member registration and maintenance (linked list) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "library.h"

static void printMemberHeader(void) {
    printf("\n%-6s %-25s %-16s %-30s\n", "ID", "Name", "Phone", "Email");
    printf("----------------------------------------------------------------------------\n");
}

static void printMember(const Member *m) {
    printf("%-6d %-25.25s %-16.16s %-30.30s\n", m->id, m->name, m->phone, m->email);
}

void registerMember(void) {
    Member m;
    int id;

    if (!readInt("Enter Member ID: ", &id) || id <= 0) {
        printf("Invalid Member ID. It must be a positive number.\n"); return;
    }
    if (findMember(id)) {
        printf("Error: Member ID %d already exists.\n", id); return;
    }
    m.id = id;

    readLine("Enter Name: ", m.name, sizeof m.name);
    if (isBlank(m.name)) { printf("Name cannot be empty.\n"); return; }
    readLine("Enter Phone: ", m.phone, sizeof m.phone);
    if (!validPhone(m.phone)) { printf("Invalid phone number.\n"); return; }
    readLine("Enter Email: ", m.email, sizeof m.email);
    if (!validEmail(m.email)) { printf("Invalid email address.\n"); return; }

    Member *node = malloc(sizeof(Member));
    if (!node) { printf("Out of memory.\n"); return; }
    *node = m;
    appendMember(node);
    saveAll();
    printf("Member registered successfully.\n");
}

void viewMembers(void) {
    if (!memberHead) { printf("No members registered.\n"); return; }
    printMemberHeader();
    for (Member *p = memberHead; p; p = p->next) printMember(p);
}

void searchMember(void) {
    printf("\nSearch by: 1.Member ID  2.Name\n");
    int choice;
    if (!readInt("Choice: ", &choice) || (choice != 1 && choice != 2)) {
        printf("Invalid choice.\n"); return;
    }
    int found = 0;
    if (choice == 1) {
        int id;
        if (!readInt("Enter Member ID: ", &id)) { printf("Invalid ID.\n"); return; }
        Member *m = findMember(id);
        if (m) { printMemberHeader(); printMember(m); found = 1; }
    } else {
        char key[60];
        readLine("Enter name: ", key, sizeof key);
        if (isBlank(key)) { printf("Search text cannot be empty.\n"); return; }
        for (Member *p = memberHead; p; p = p->next)
            if (containsIgnoreCase(p->name, key)) {
                if (!found) printMemberHeader();
                printMember(p);
                found = 1;
            }
    }
    if (!found) printf("No matching members found.\n");
}

void updateMember(void) {
    int id;
    if (!readInt("Enter Member ID to update: ", &id)) {
        printf("Invalid Member ID.\n"); return;
    }
    Member *m = findMember(id);
    if (!m) { printf("Member not found.\n"); return; }

    char buf[80];
    printf("(Press Enter to keep the current value)\n");

    printf("Name [%s]\n", m->name);
    readLine("New Name: ", buf, sizeof buf);
    if (!isBlank(buf)) copyStr(m->name, buf, sizeof m->name);

    printf("Phone [%s]\n", m->phone);
    readLine("New Phone: ", buf, sizeof buf);
    if (!isBlank(buf)) {
        if (validPhone(buf)) copyStr(m->phone, buf, sizeof m->phone);
        else printf("Invalid phone; not changed.\n");
    }

    printf("Email [%s]\n", m->email);
    readLine("New Email: ", buf, sizeof buf);
    if (!isBlank(buf)) {
        if (validEmail(buf)) copyStr(m->email, buf, sizeof m->email);
        else printf("Invalid email; not changed.\n");
    }
    saveAll();
    printf("Member updated successfully.\n");
}

void deleteMember(void) {
    int id;
    if (!readInt("Enter Member ID to delete: ", &id)) {
        printf("Invalid Member ID.\n"); return;
    }
    if (!findMember(id)) { printf("Member not found.\n"); return; }
    if (memberHasOpenLoans(id)) {
        printf("Cannot delete: member still has books issued.\n"); return;
    }

    /* unlink the node, keeping track of the previous one */
    Member *prev = NULL, *cur = memberHead;
    while (cur && cur->id != id) { prev = cur; cur = cur->next; }
    if (prev) prev->next = cur->next;
    else      memberHead = cur->next;
    free(cur);

    saveAll();
    printf("Member deleted successfully.\n");
}

void memberMenu(void) {
    int c;
    do {
        printf("\n--- MEMBER MANAGEMENT ---\n"
               "1. Register Member\n2. View All Members\n3. Search Member\n"
               "4. Update Member\n5. Delete Member\n0. Back\n");
        if (!readInt("Choice: ", &c)) { printf("Invalid input.\n"); c = -1; continue; }
        switch (c) {
            case 1: registerMember(); break;
            case 2: viewMembers(); break;
            case 3: searchMember(); break;
            case 4: updateMember(); break;
            case 5: deleteMember(); break;
            case 0: break;
            default: printf("Invalid choice.\n");
        }
    } while (c != 0);
}
