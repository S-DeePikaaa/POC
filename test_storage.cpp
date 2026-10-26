/* test_storage.cpp - tests for every function in storage.c */
#include "test_helpers.h"

class StorageTest : public LibraryTest {};

/* ============================ appendBook ============================ */

TEST_F(StorageTest, AppendBook_FirstNodeBecomesHead) {
    Book *b = (Book *)calloc(1, sizeof(Book));
    b->id = 1;
    appendBook(b);
    EXPECT_EQ(bookHead, b);
    EXPECT_EQ(b->next, nullptr);
}

TEST_F(StorageTest, AppendBook_KeepsInsertionOrder) {
    Book *a = mkBook(1, "A", "x", "c", 1, 1);
    Book *b = mkBook(2, "B", "x", "c", 1, 1);
    Book *c = mkBook(3, "C", "x", "c", 1, 1);
    EXPECT_EQ(bookHead, a);
    EXPECT_EQ(a->next, b);
    EXPECT_EQ(b->next, c);
    EXPECT_EQ(c->next, nullptr);
}

TEST_F(StorageTest, AppendBook_ResetsStaleNextPointer) {
    mkBook(1, "A", "x", "c", 1, 1);
    Book *b = (Book *)calloc(1, sizeof(Book));
    b->id = 2;
    b->next = (Book *)0x1;           /* garbage */
    appendBook(b);
    EXPECT_EQ(b->next, nullptr);
    EXPECT_EQ(countBooks(), 2);
}

/* =========================== appendMember =========================== */

TEST_F(StorageTest, AppendMember_FirstNodeBecomesHead) {
    Member *m = (Member *)calloc(1, sizeof(Member));
    appendMember(m);
    EXPECT_EQ(memberHead, m);
    EXPECT_EQ(m->next, nullptr);
}

TEST_F(StorageTest, AppendMember_KeepsInsertionOrder) {
    Member *a = mkMember(1, "A", "1234567", "a@b.com");
    Member *b = mkMember(2, "B", "1234567", "b@b.com");
    EXPECT_EQ(memberHead, a);
    EXPECT_EQ(a->next, b);
    EXPECT_EQ(b->next, nullptr);
}

TEST_F(StorageTest, AppendMember_ResetsStaleNextPointer) {
    mkMember(1, "A", "1234567", "a@b.com");
    Member *m = (Member *)calloc(1, sizeof(Member));
    m->next = (Member *)0x1;
    appendMember(m);
    EXPECT_EQ(m->next, nullptr);
}

/* ======================== appendTransaction ========================= */

TEST_F(StorageTest, AppendTransaction_FirstNodeBecomesHead) {
    Transaction *t = (Transaction *)calloc(1, sizeof(Transaction));
    appendTransaction(t);
    EXPECT_EQ(transHead, t);
    EXPECT_EQ(t->next, nullptr);
}

TEST_F(StorageTest, AppendTransaction_KeepsInsertionOrder) {
    Transaction *a = mkTrans(1, 1, 1, 0, 0, 0, 0, 0);
    Transaction *b = mkTrans(2, 1, 1, 0, 0, 0, 0, 0);
    Transaction *c = mkTrans(3, 1, 1, 0, 0, 0, 0, 0);
    EXPECT_EQ(transHead, a);
    EXPECT_EQ(a->next, b);
    EXPECT_EQ(b->next, c);
    EXPECT_EQ(c->next, nullptr);
}

TEST_F(StorageTest, AppendTransaction_ResetsStaleNextPointer) {
    mkTrans(1, 1, 1, 0, 0, 0, 0, 0);
    Transaction *t = (Transaction *)calloc(1, sizeof(Transaction));
    t->next = (Transaction *)0x1;
    appendTransaction(t);
    EXPECT_EQ(t->next, nullptr);
}

/* ============================== saveAll ============================= */

TEST_F(StorageTest, SaveAll_EmptyListsCreateEmptyFiles) {
    saveAll();
    ASSERT_TRUE(fs::exists(BOOK_FILE));
    ASSERT_TRUE(fs::exists(MEMBER_FILE));
    ASSERT_TRUE(fs::exists(TRANS_FILE));
    EXPECT_EQ(readFile(BOOK_FILE), "");
    EXPECT_EQ(readFile(MEMBER_FILE), "");
    EXPECT_EQ(readFile(TRANS_FILE), "");
}

TEST_F(StorageTest, SaveAll_WritesBookLines) {
    mkBook(1, "C Programming", "K&R", "Tech", 5, 3);
    mkBook(2, "Dune", "Herbert", "SciFi", 2, 2);
    saveAll();
    EXPECT_EQ(readFile(BOOK_FILE),
              "1|C Programming|K&R|Tech|5|3\n"
              "2|Dune|Herbert|SciFi|2|2\n");
}

TEST_F(StorageTest, SaveAll_ReplacesPipeInTextFields) {
    mkBook(1, "A|B", "C|D", "E|F", 1, 1);
    mkMember(1, "N|M", "1234567", "a|b@c.com");
    saveAll();
    EXPECT_EQ(readFile(BOOK_FILE), "1|A/B|C/D|E/F|1|1\n");
    EXPECT_EQ(readFile(MEMBER_FILE), "1|N/M|1234567|a/b@c.com\n");
}

TEST_F(StorageTest, SaveAll_WritesMemberLines) {
    mkMember(10, "John Doe", "9876543210", "john@x.com");
    saveAll();
    EXPECT_EQ(readFile(MEMBER_FILE), "10|John Doe|9876543210|john@x.com\n");
}

TEST_F(StorageTest, SaveAll_WritesTransactionLines) {
    mkTrans(1, 2, 3, D(1, 1, 2024), D(15, 1, 2024), 0, 0, 0.0f);
    mkTrans(2, 2, 3, D(1, 1, 2024), D(15, 1, 2024), D(20, 1, 2024), 1, 10.0f);
    saveAll();
    EXPECT_EQ(readFile(TRANS_FILE),
              "1|2|3|01-01-2024|15-01-2024|-|0|0.00\n"
              "2|2|3|01-01-2024|15-01-2024|20-01-2024|1|10.00\n");
}

TEST_F(StorageTest, SaveAll_OverwritesPreviousContent) {
    mkBook(1, "A", "B", "C", 1, 1);
    saveAll();
    freeAll();
    mkBook(2, "X", "Y", "Z", 1, 1);
    saveAll();
    EXPECT_EQ(readFile(BOOK_FILE), "2|X|Y|Z|1|1\n");
}

TEST_F(StorageTest, SaveAll_WarnsWhenFilesCannotBeWritten) {
    /* a directory with the file's name makes fopen(..., "w") fail */
    fs::create_directory(BOOK_FILE);
    fs::create_directory(MEMBER_FILE);
    fs::create_directory(TRANS_FILE);
    testing::internal::CaptureStdout();
    saveAll();
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(has(out, "Warning: could not save books."));
    EXPECT_TRUE(has(out, "Warning: could not save members."));
    EXPECT_TRUE(has(out, "Warning: could not save transactions."));
}

/* ============================== loadAll ============================= */

TEST_F(StorageTest, LoadAll_MissingFilesLeaveListsEmpty) {
    loadAll();
    EXPECT_EQ(bookHead, nullptr);
    EXPECT_EQ(memberHead, nullptr);
    EXPECT_EQ(transHead, nullptr);
}

TEST_F(StorageTest, LoadAll_LoadsBooks) {
    writeFile(BOOK_FILE, "1|Title One|Auth One|Cat|5|3\n2|Two|Auth2|Cat2|1|0\n");
    loadAll();
    ASSERT_EQ(countBooks(), 2);
    EXPECT_EQ(bookHead->id, 1);
    EXPECT_STREQ(bookHead->title, "Title One");
    EXPECT_STREQ(bookHead->author, "Auth One");
    EXPECT_STREQ(bookHead->category, "Cat");
    EXPECT_EQ(bookHead->totalCopies, 5);
    EXPECT_EQ(bookHead->availableCopies, 3);
    EXPECT_EQ(bookHead->next->id, 2);
    EXPECT_EQ(bookHead->next->availableCopies, 0);
}

TEST_F(StorageTest, LoadAll_LoadsMembers) {
    writeFile(MEMBER_FILE, "7|Jane|9876543210|jane@x.com\n");
    loadAll();
    ASSERT_EQ(countMembers(), 1);
    EXPECT_EQ(memberHead->id, 7);
    EXPECT_STREQ(memberHead->name, "Jane");
    EXPECT_STREQ(memberHead->phone, "9876543210");
    EXPECT_STREQ(memberHead->email, "jane@x.com");
}

TEST_F(StorageTest, LoadAll_LoadsTransactions) {
    writeFile(TRANS_FILE,
              "1|2|3|01-01-2024|15-01-2024|-|0|0.00\n"
              "2|2|3|01-01-2024|15-01-2024|20-01-2024|1|10.00\n");
    loadAll();
    ASSERT_EQ(countTrans(), 2);
    Transaction *t1 = transHead, *t2 = transHead->next;
    EXPECT_EQ(t1->transId, 1);
    EXPECT_EQ(t1->bookId, 2);
    EXPECT_EQ(t1->memberId, 3);
    EXPECT_EQ(t1->issueDate, D(1, 1, 2024));
    EXPECT_EQ(t1->expectedReturn, D(15, 1, 2024));
    EXPECT_EQ(t1->returnDate, 0);
    EXPECT_EQ(t1->returned, 0);
    EXPECT_FLOAT_EQ(t1->fine, 0.0f);
    EXPECT_EQ(t2->returnDate, D(20, 1, 2024));
    EXPECT_EQ(t2->returned, 1);
    EXPECT_FLOAT_EQ(t2->fine, 10.0f);
}

TEST_F(StorageTest, LoadAll_SkipsMalformedLines) {
    writeFile(BOOK_FILE,
              "1|Good|A|C|1|1\n"
              "2|TooFewFields|A|C|1\n"
              "4|Also Good|A|C|2|2\n");
    loadAll();
    ASSERT_EQ(countBooks(), 2);
    EXPECT_EQ(bookHead->id, 1);
    EXPECT_EQ(bookHead->next->id, 4);
}

TEST_F(StorageTest, LoadAll_IgnoresBlankLinesAndHandlesCRLF) {
    writeFile(BOOK_FILE, "\n1|T|A|C|1|1\r\n   \r\n2|U|B|D|2|2\r\n\n");
    loadAll();
    ASSERT_EQ(countBooks(), 2);
    EXPECT_EQ(bookHead->availableCopies, 1);
    EXPECT_EQ(bookHead->next->availableCopies, 2);
}

TEST_F(StorageTest, LoadAll_SkipsTransactionsWithBadDates) {
    writeFile(TRANS_FILE,
              "1|1|1|31-02-2024|15-01-2024|-|0|0.00\n"
              "2|1|1|01-01-2024|bad|-|0|0.00\n"
              "3|1|1|01-01-2024|15-01-2024|-|0|0.00\n");
    loadAll();
    ASSERT_EQ(countTrans(), 1);
    EXPECT_EQ(transHead->transId, 3);
}

TEST_F(StorageTest, LoadAll_TruncatesOverlongFields) {
    std::string longTitle(150, 'x');
    writeFile(BOOK_FILE, "1|" + longTitle + "|A|C|1|1\n");
    loadAll();
    ASSERT_EQ(countBooks(), 1);
    EXPECT_EQ(strlen(bookHead->title), sizeof bookHead->title - 1);
}

TEST_F(StorageTest, LoadAll_RoundTripsWithSaveAll) {
    mkBook(1, "Alpha", "Auth", "Cat", 4, 2);
    mkMember(5, "Bob", "1234567890", "bob@x.com");
    mkTrans(9, 1, 5, D(2, 2, 2024), D(16, 2, 2024), D(18, 2, 2024), 1, 4.0f);
    saveAll();
    freeAll();
    ASSERT_EQ(bookHead, nullptr);

    loadAll();
    ASSERT_EQ(countBooks(), 1);
    ASSERT_EQ(countMembers(), 1);
    ASSERT_EQ(countTrans(), 1);
    EXPECT_STREQ(bookHead->title, "Alpha");
    EXPECT_EQ(bookHead->availableCopies, 2);
    EXPECT_STREQ(memberHead->email, "bob@x.com");
    EXPECT_EQ(transHead->issueDate, D(2, 2, 2024));
    EXPECT_EQ(transHead->returnDate, D(18, 2, 2024));
    EXPECT_FLOAT_EQ(transHead->fine, 4.0f);
}

/* ============================== freeAll ============================= */

TEST_F(StorageTest, FreeAll_ClearsAllLists) {
    mkBook(1, "A", "B", "C", 1, 1);
    mkMember(1, "N", "1234567", "a@b.com");
    mkTrans(1, 1, 1, 0, 0, 0, 0, 0);
    freeAll();
    EXPECT_EQ(bookHead, nullptr);
    EXPECT_EQ(memberHead, nullptr);
    EXPECT_EQ(transHead, nullptr);
}

TEST_F(StorageTest, FreeAll_SafeOnEmptyLists) {
    EXPECT_NO_FATAL_FAILURE(freeAll());
    EXPECT_EQ(bookHead, nullptr);
}

TEST_F(StorageTest, FreeAll_CanBeCalledTwice) {
    mkBook(1, "A", "B", "C", 1, 1);
    freeAll();
    EXPECT_NO_FATAL_FAILURE(freeAll());
}

/* ============================== findBook ============================ */

TEST_F(StorageTest, FindBook_EmptyList) {
    EXPECT_EQ(findBook(1), nullptr);
}

TEST_F(StorageTest, FindBook_FindsExistingIds) {
    Book *a = mkBook(1, "A", "x", "c", 1, 1);
    Book *b = mkBook(2, "B", "x", "c", 1, 1);
    Book *c = mkBook(3, "C", "x", "c", 1, 1);
    EXPECT_EQ(findBook(1), a);
    EXPECT_EQ(findBook(2), b);
    EXPECT_EQ(findBook(3), c);
}

TEST_F(StorageTest, FindBook_MissingIdReturnsNull) {
    mkBook(1, "A", "x", "c", 1, 1);
    EXPECT_EQ(findBook(99), nullptr);
}

/* ============================= findMember =========================== */

TEST_F(StorageTest, FindMember_EmptyList) {
    EXPECT_EQ(findMember(1), nullptr);
}

TEST_F(StorageTest, FindMember_FindsExistingIds) {
    Member *a = mkMember(1, "A", "1234567", "a@b.com");
    Member *b = mkMember(2, "B", "1234567", "b@b.com");
    EXPECT_EQ(findMember(1), a);
    EXPECT_EQ(findMember(2), b);
}

TEST_F(StorageTest, FindMember_MissingIdReturnsNull) {
    mkMember(1, "A", "1234567", "a@b.com");
    EXPECT_EQ(findMember(42), nullptr);
}

/* ========================= memberHasOpenLoans ======================= */

TEST_F(StorageTest, MemberOpenLoans_NoTransactions) {
    EXPECT_EQ(memberHasOpenLoans(1), 0);
}

TEST_F(StorageTest, MemberOpenLoans_TrueForUnreturned) {
    mkTrans(1, 10, 1, 0, 0, 0, 0, 0);
    EXPECT_EQ(memberHasOpenLoans(1), 1);
}

TEST_F(StorageTest, MemberOpenLoans_FalseWhenAllReturned) {
    mkTrans(1, 10, 1, 0, 0, 0, 1, 0);
    EXPECT_EQ(memberHasOpenLoans(1), 0);
}

TEST_F(StorageTest, MemberOpenLoans_IgnoresOtherMembers) {
    mkTrans(1, 10, 2, 0, 0, 0, 0, 0);
    EXPECT_EQ(memberHasOpenLoans(1), 0);
    EXPECT_EQ(memberHasOpenLoans(2), 1);
}

TEST_F(StorageTest, MemberOpenLoans_FindsOpenAmongReturned) {
    mkTrans(1, 10, 1, 0, 0, 0, 1, 0);
    mkTrans(2, 11, 1, 0, 0, 0, 0, 0);
    EXPECT_EQ(memberHasOpenLoans(1), 1);
}

/* ========================== bookHasOpenLoans ======================== */

TEST_F(StorageTest, BookOpenLoans_NoTransactions) {
    EXPECT_EQ(bookHasOpenLoans(1), 0);
}

TEST_F(StorageTest, BookOpenLoans_TrueForUnreturned) {
    mkTrans(1, 10, 1, 0, 0, 0, 0, 0);
    EXPECT_EQ(bookHasOpenLoans(10), 1);
}

TEST_F(StorageTest, BookOpenLoans_FalseWhenReturned) {
    mkTrans(1, 10, 1, 0, 0, 0, 1, 0);
    EXPECT_EQ(bookHasOpenLoans(10), 0);
}

TEST_F(StorageTest, BookOpenLoans_IgnoresOtherBooks) {
    mkTrans(1, 11, 1, 0, 0, 0, 0, 0);
    EXPECT_EQ(bookHasOpenLoans(10), 0);
    EXPECT_EQ(bookHasOpenLoans(11), 1);
}
