/* test_book.cpp - tests for every function in book.c */
#include "test_helpers.h"

class BookTest : public LibraryTest {};

/* ============================== addBook ============================= */

TEST_F(BookTest, AddBook_ValidInputAddsBookAndSaves) {
    std::string out = run(addBook, "1\nC Programming\nK&R\nTech\n5\n");
    EXPECT_TRUE(has(out, "Book added successfully."));
    ASSERT_EQ(countBooks(), 1);
    EXPECT_EQ(bookHead->id, 1);
    EXPECT_STREQ(bookHead->title, "C Programming");
    EXPECT_STREQ(bookHead->author, "K&R");
    EXPECT_STREQ(bookHead->category, "Tech");
    EXPECT_EQ(bookHead->totalCopies, 5);
    EXPECT_EQ(bookHead->availableCopies, 5);
    EXPECT_EQ(readFile(BOOK_FILE), "1|C Programming|K&R|Tech|5|5\n");
}

TEST_F(BookTest, AddBook_AppendsToEnd) {
    mkBook(1, "A", "B", "C", 1, 1);
    run(addBook, "2\nT\nA\nC\n1\n");
    ASSERT_EQ(countBooks(), 2);
    EXPECT_EQ(bookHead->next->id, 2);
}

TEST_F(BookTest, AddBook_RejectsNonNumericId) {
    std::string out = run(addBook, "abc\n");
    EXPECT_TRUE(has(out, "Invalid Book ID"));
    EXPECT_EQ(countBooks(), 0);
}

TEST_F(BookTest, AddBook_RejectsZeroAndNegativeId) {
    EXPECT_TRUE(has(run(addBook, "0\n"), "Invalid Book ID"));
    EXPECT_TRUE(has(run(addBook, "-4\n"), "Invalid Book ID"));
    EXPECT_EQ(countBooks(), 0);
}

TEST_F(BookTest, AddBook_RejectsDuplicateId) {
    mkBook(1, "A", "B", "C", 1, 1);
    std::string out = run(addBook, "1\nT\nA\nC\n1\n");
    EXPECT_TRUE(has(out, "Book ID 1 already exists"));
    EXPECT_EQ(countBooks(), 1);
}

TEST_F(BookTest, AddBook_RejectsBlankTitle) {
    std::string out = run(addBook, "1\n   \nA\nC\n1\n");
    EXPECT_TRUE(has(out, "Title cannot be empty."));
    EXPECT_EQ(countBooks(), 0);
}

TEST_F(BookTest, AddBook_RejectsBlankAuthor) {
    std::string out = run(addBook, "1\nT\n\nC\n1\n");
    EXPECT_TRUE(has(out, "Author cannot be empty."));
    EXPECT_EQ(countBooks(), 0);
}

TEST_F(BookTest, AddBook_RejectsBlankCategory) {
    std::string out = run(addBook, "1\nT\nA\n\n1\n");
    EXPECT_TRUE(has(out, "Category cannot be empty."));
    EXPECT_EQ(countBooks(), 0);
}

TEST_F(BookTest, AddBook_RejectsBadCopyCount) {
    EXPECT_TRUE(has(run(addBook, "1\nT\nA\nC\n0\n"), "Invalid quantity"));
    EXPECT_TRUE(has(run(addBook, "1\nT\nA\nC\n-2\n"), "Invalid quantity"));
    EXPECT_TRUE(has(run(addBook, "1\nT\nA\nC\nmany\n"), "Invalid quantity"));
    EXPECT_EQ(countBooks(), 0);
}

/* ============================= viewBooks ============================ */

TEST_F(BookTest, ViewBooks_EmptyLibrary) {
    EXPECT_TRUE(has(run(viewBooks), "No books in the library."));
}

TEST_F(BookTest, ViewBooks_ListsHeaderAndBooks) {
    mkBook(1, "Dune", "Herbert", "SciFi", 3, 2);
    mkBook(2, "Emma", "Austen", "Classic", 1, 0);
    std::string out = run(viewBooks);
    EXPECT_TRUE(has(out, "Title"));
    EXPECT_TRUE(has(out, "Available"));
    EXPECT_TRUE(has(out, "Dune"));
    EXPECT_TRUE(has(out, "Herbert"));
    EXPECT_TRUE(has(out, "Emma"));
    EXPECT_TRUE(has(out, "Classic"));
}

TEST_F(BookTest, ViewBooks_TruncatesLongTitleTo30Chars) {
    std::string title = std::string(30, 'a') + "ZZZ";
    mkBook(1, title.c_str(), "Au", "Cat", 1, 1);
    std::string out = run(viewBooks);
    EXPECT_TRUE(has(out, std::string(30, 'a')));
    EXPECT_FALSE(has(out, "ZZZ"));
}

/* ============================= updateBook =========================== */

TEST_F(BookTest, UpdateBook_InvalidId) {
    EXPECT_TRUE(has(run(updateBook, "x\n"), "Invalid Book ID."));
}

TEST_F(BookTest, UpdateBook_NotFound) {
    EXPECT_TRUE(has(run(updateBook, "5\n"), "Book not found."));
}

TEST_F(BookTest, UpdateBook_BlankAnswersKeepEverything) {
    mkBook(1, "Old", "OldA", "OldC", 5, 4);
    std::string out = run(updateBook, "1\n\n\n\n\n");
    EXPECT_TRUE(has(out, "Book updated successfully."));
    EXPECT_STREQ(bookHead->title, "Old");
    EXPECT_STREQ(bookHead->author, "OldA");
    EXPECT_STREQ(bookHead->category, "OldC");
    EXPECT_EQ(bookHead->totalCopies, 5);
    EXPECT_EQ(bookHead->availableCopies, 4);
}

TEST_F(BookTest, UpdateBook_ChangesTextFieldsAndSaves) {
    mkBook(1, "Old", "OldA", "OldC", 5, 4);
    run(updateBook, "1\nNew\nNewA\nNewC\n\n");
    EXPECT_STREQ(bookHead->title, "New");
    EXPECT_STREQ(bookHead->author, "NewA");
    EXPECT_STREQ(bookHead->category, "NewC");
    EXPECT_EQ(readFile(BOOK_FILE), "1|New|NewA|NewC|5|4\n");
}

TEST_F(BookTest, UpdateBook_IncreaseTotalAdjustsAvailable) {
    mkBook(1, "T", "A", "C", 5, 2);       /* 3 issued */
    run(updateBook, "1\n\n\n\n10\n");
    EXPECT_EQ(bookHead->totalCopies, 10);
    EXPECT_EQ(bookHead->availableCopies, 7);
}

TEST_F(BookTest, UpdateBook_CannotGoBelowIssuedCount) {
    mkBook(1, "T", "A", "C", 5, 2);       /* 3 issued */
    std::string out = run(updateBook, "1\n\n\n\n2\n");
    EXPECT_TRUE(has(out, "Cannot set copies below the 3 currently issued"));
    EXPECT_EQ(bookHead->totalCopies, 5);
    EXPECT_EQ(bookHead->availableCopies, 2);
}

TEST_F(BookTest, UpdateBook_TotalEqualToIssuedIsAllowed) {
    mkBook(1, "T", "A", "C", 5, 2);
    run(updateBook, "1\n\n\n\n3\n");
    EXPECT_EQ(bookHead->totalCopies, 3);
    EXPECT_EQ(bookHead->availableCopies, 0);
}

TEST_F(BookTest, UpdateBook_RejectsInvalidQuantity) {
    mkBook(1, "T", "A", "C", 5, 5);
    EXPECT_TRUE(has(run(updateBook, "1\n\n\n\nabc\n"), "Invalid quantity"));
    EXPECT_TRUE(has(run(updateBook, "1\n\n\n\n0\n"), "Invalid quantity"));
    EXPECT_TRUE(has(run(updateBook, "1\n\n\n\n-3\n"), "Invalid quantity"));
    EXPECT_EQ(bookHead->totalCopies, 5);
}

/* ============================= deleteBook =========================== */

TEST_F(BookTest, DeleteBook_InvalidId) {
    EXPECT_TRUE(has(run(deleteBook, "abc\n"), "Invalid Book ID."));
}

TEST_F(BookTest, DeleteBook_NotFound) {
    EXPECT_TRUE(has(run(deleteBook, "3\n"), "Book not found."));
}

TEST_F(BookTest, DeleteBook_BlockedByOpenLoan) {
    mkBook(1, "T", "A", "C", 1, 0);
    mkTrans(1, 1, 1, D(1, 1, 2024), D(15, 1, 2024), 0, 0, 0);
    std::string out = run(deleteBook, "1\n");
    EXPECT_TRUE(has(out, "Cannot delete"));
    EXPECT_EQ(countBooks(), 1);
}

TEST_F(BookTest, DeleteBook_AllowedAfterReturn) {
    mkBook(1, "T", "A", "C", 1, 1);
    mkTrans(1, 1, 1, D(1, 1, 2024), D(15, 1, 2024), D(10, 1, 2024), 1, 0);
    std::string out = run(deleteBook, "1\n");
    EXPECT_TRUE(has(out, "Book deleted successfully."));
    EXPECT_EQ(countBooks(), 0);
}

TEST_F(BookTest, DeleteBook_Head) {
    mkBook(1, "A", "x", "c", 1, 1);
    Book *b = mkBook(2, "B", "x", "c", 1, 1);
    run(deleteBook, "1\n");
    EXPECT_EQ(bookHead, b);
    EXPECT_EQ(countBooks(), 1);
}

TEST_F(BookTest, DeleteBook_Middle) {
    Book *a = mkBook(1, "A", "x", "c", 1, 1);
    mkBook(2, "B", "x", "c", 1, 1);
    Book *c = mkBook(3, "C", "x", "c", 1, 1);
    run(deleteBook, "2\n");
    EXPECT_EQ(bookHead, a);
    EXPECT_EQ(a->next, c);
    EXPECT_EQ(findBook(2), nullptr);
}

TEST_F(BookTest, DeleteBook_Tail) {
    Book *a = mkBook(1, "A", "x", "c", 1, 1);
    Book *b = mkBook(2, "B", "x", "c", 1, 1);
    mkBook(3, "C", "x", "c", 1, 1);
    run(deleteBook, "3\n");
    EXPECT_EQ(b->next, nullptr);
    EXPECT_EQ(a->next, b);
}

TEST_F(BookTest, DeleteBook_OnlyBookEmptiesList) {
    mkBook(1, "A", "x", "c", 1, 1);
    run(deleteBook, "1\n");
    EXPECT_EQ(bookHead, nullptr);
    EXPECT_EQ(readFile(BOOK_FILE), "");
}

/* ============================ searchBooks =========================== */

class SearchBooksTest : public BookTest {
protected:
    void SetUp() override {
        BookTest::SetUp();
        mkBook(1, "The Great Gatsby", "F. Scott Fitzgerald", "Classic", 2, 2);
        mkBook(2, "Dune", "Frank Herbert", "SciFi", 1, 0);
        mkBook(3, "Neuromancer", "William Gibson", "SciFi", 1, 1);
    }
};

TEST_F(SearchBooksTest, InvalidChoice) {
    EXPECT_TRUE(has(run(searchBooks, "9\n"), "Invalid choice."));
    EXPECT_TRUE(has(run(searchBooks, "0\n"), "Invalid choice."));
    EXPECT_TRUE(has(run(searchBooks, "x\n"), "Invalid choice."));
}

TEST_F(SearchBooksTest, ById_Available) {
    std::string out = run(searchBooks, "1\n1\n");
    EXPECT_TRUE(has(out, "Great Gatsby"));
    EXPECT_TRUE(has(out, "Available for issue"));
    EXPECT_FALSE(has(out, "Dune"));
}

TEST_F(SearchBooksTest, ById_NotAvailable) {
    std::string out = run(searchBooks, "1\n2\n");
    EXPECT_TRUE(has(out, "Dune"));
    EXPECT_TRUE(has(out, "Not available (all copies issued)"));
}

TEST_F(SearchBooksTest, ById_InvalidNumber) {
    EXPECT_TRUE(has(run(searchBooks, "1\nabc\n"), "Invalid Book ID."));
}

TEST_F(SearchBooksTest, ById_NoMatch) {
    EXPECT_TRUE(has(run(searchBooks, "1\n99\n"), "No matching books found."));
}

TEST_F(SearchBooksTest, ByTitle_CaseInsensitivePartial) {
    std::string out = run(searchBooks, "2\ngREAT\n");
    EXPECT_TRUE(has(out, "Great Gatsby"));
    EXPECT_FALSE(has(out, "Neuromancer"));
}

TEST_F(SearchBooksTest, ByAuthor) {
    std::string out = run(searchBooks, "3\nfrank\n");
    EXPECT_TRUE(has(out, "Dune"));
    EXPECT_FALSE(has(out, "Gatsby"));
}

TEST_F(SearchBooksTest, ByCategory_ReturnsAllMatches) {
    std::string out = run(searchBooks, "4\nscifi\n");
    EXPECT_TRUE(has(out, "Dune"));
    EXPECT_TRUE(has(out, "Neuromancer"));
    EXPECT_FALSE(has(out, "Gatsby"));
}

TEST_F(SearchBooksTest, BlankSearchText) {
    EXPECT_TRUE(has(run(searchBooks, "2\n   \n"), "Search text cannot be empty."));
}

TEST_F(SearchBooksTest, NoMatchForText) {
    EXPECT_TRUE(has(run(searchBooks, "2\nzzzz\n"), "No matching books found."));
}

/* ========================= checkAvailability ======================== */

TEST_F(BookTest, CheckAvailability_InvalidId) {
    EXPECT_TRUE(has(run(checkAvailability, "x\n"), "Invalid Book ID."));
}

TEST_F(BookTest, CheckAvailability_NotFound) {
    EXPECT_TRUE(has(run(checkAvailability, "7\n"), "Book not found."));
}

TEST_F(BookTest, CheckAvailability_Available) {
    mkBook(1, "Dune", "H", "S", 3, 2);
    std::string out = run(checkAvailability, "1\n");
    EXPECT_TRUE(has(out, "'Dune' : 2 of 3 copies available -> AVAILABLE"));
    EXPECT_FALSE(has(out, "NOT AVAILABLE"));
}

TEST_F(BookTest, CheckAvailability_NotAvailable) {
    mkBook(1, "Dune", "H", "S", 3, 0);
    std::string out = run(checkAvailability, "1\n");
    EXPECT_TRUE(has(out, "0 of 3 copies available -> NOT AVAILABLE"));
}

/* ============================== bookMenu ============================ */

TEST_F(BookTest, BookMenu_ExitsOnZero) {
    std::string out = run(bookMenu, "0\n");
    EXPECT_TRUE(has(out, "BOOK MANAGEMENT"));
}

TEST_F(BookTest, BookMenu_InvalidTextThenExit) {
    EXPECT_TRUE(has(run(bookMenu, "abc\n0\n"), "Invalid input."));
}

TEST_F(BookTest, BookMenu_UnknownNumberThenExit) {
    EXPECT_TRUE(has(run(bookMenu, "9\n0\n"), "Invalid choice."));
}

TEST_F(BookTest, BookMenu_AddThenViewThenExit) {
    std::string out = run(bookMenu, "1\n1\nMenu Book\nAuth\nCat\n2\n2\n0\n");
    EXPECT_TRUE(has(out, "Book added successfully."));
    EXPECT_TRUE(has(out, "Menu Book"));
    EXPECT_EQ(countBooks(), 1);
}

TEST_F(BookTest, BookMenu_UpdateOption) {
    mkBook(1, "Old", "A", "C", 1, 1);
    run(bookMenu, "3\n1\nNew\n\n\n\n0\n");
    EXPECT_STREQ(bookHead->title, "New");
}

TEST_F(BookTest, BookMenu_DeleteOption) {
    mkBook(1, "Old", "A", "C", 1, 1);
    run(bookMenu, "4\n1\n0\n");
    EXPECT_EQ(countBooks(), 0);
}
