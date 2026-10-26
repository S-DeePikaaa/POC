/* test_transaction.cpp - tests for every function in transaction.c.
 * (nextTransId is static, so it is covered through issueBook.) */
#include "test_helpers.h"

static std::string money(float v) {
    char b[32];
    snprintf(b, sizeof b, "%.2f", v);
    return b;
}

/* ============================= issueBook ============================ */

class IssueBookTest : public LibraryTest {
protected:
    void SetUp() override {
        LibraryTest::SetUp();
        mkBook(1, "Dune", "Herbert", "SciFi", 2, 2);
        mkMember(1, "Alice", "1234567", "a@x.com");
    }
};

TEST_F(IssueBookTest, IssuesBookWithGivenDate) {
    std::string out = run(issueBook, "1\n1\n01-01-2024\n");
    EXPECT_TRUE(has(out, "Book issued. Transaction ID: 1"));
    EXPECT_TRUE(has(out, "Issue date: 01-01-2024"));
    EXPECT_TRUE(has(out, "Expected return: 15-01-2024"));
    EXPECT_TRUE(has(out, "Copies left: 1"));
    ASSERT_EQ(countTrans(), 1);
    EXPECT_EQ(transHead->bookId, 1);
    EXPECT_EQ(transHead->memberId, 1);
    EXPECT_EQ(transHead->issueDate, D(1, 1, 2024));
    EXPECT_EQ(transHead->expectedReturn, D(15, 1, 2024));
    EXPECT_EQ(transHead->returned, 0);
    EXPECT_EQ(transHead->returnDate, 0);
    EXPECT_FLOAT_EQ(transHead->fine, 0.0f);
    EXPECT_EQ(findBook(1)->availableCopies, 1);
}

TEST_F(IssueBookTest, DueDateRespectsLoanPeriodAcrossLeapMonth) {
    run(issueBook, "1\n1\n20-02-2024\n");
    EXPECT_EQ(transHead->expectedReturn, D(5, 3, 2024));
}

TEST_F(IssueBookTest, BlankDateMeansToday) {
    run(issueBook, "1\n1\n\n");
    ASSERT_EQ(countTrans(), 1);
    EXPECT_EQ(transHead->issueDate, today());
}

TEST_F(IssueBookTest, PersistsToFile) {
    run(issueBook, "1\n1\n01-01-2024\n");
    EXPECT_EQ(readFile(TRANS_FILE), "1|1|1|01-01-2024|15-01-2024|-|0|0.00\n");
    EXPECT_EQ(readFile(BOOK_FILE), "1|Dune|Herbert|SciFi|2|1\n");
}

TEST_F(IssueBookTest, TransactionIdIsMaxPlusOne) {
    mkTrans(7, 1, 99, D(1, 1, 2024), D(15, 1, 2024), D(2, 1, 2024), 1, 0);
    std::string out = run(issueBook, "1\n1\n01-02-2024\n");
    EXPECT_TRUE(has(out, "Transaction ID: 8"));
}

TEST_F(IssueBookTest, InvalidBookId) {
    EXPECT_TRUE(has(run(issueBook, "abc\n"), "Invalid Book ID."));
    EXPECT_EQ(countTrans(), 0);
}

TEST_F(IssueBookTest, BookNotFound) {
    EXPECT_TRUE(has(run(issueBook, "99\n1\n"), "Book not found."));
    EXPECT_EQ(countTrans(), 0);
}

TEST_F(IssueBookTest, InvalidMemberId) {
    EXPECT_TRUE(has(run(issueBook, "1\nxyz\n"), "Invalid Member ID."));
    EXPECT_EQ(countTrans(), 0);
}

TEST_F(IssueBookTest, MemberNotRegistered) {
    EXPECT_TRUE(has(run(issueBook, "1\n42\n"), "Member not registered."));
    EXPECT_EQ(countTrans(), 0);
}

TEST_F(IssueBookTest, NoCopiesAvailable) {
    findBook(1)->availableCopies = 0;
    std::string out = run(issueBook, "1\n1\n");
    EXPECT_TRUE(has(out, "Sorry, 'Dune' is currently unavailable."));
    EXPECT_EQ(countTrans(), 0);
}

TEST_F(IssueBookTest, SameMemberCannotBorrowSameTitleTwice) {
    mkTrans(1, 1, 1, D(1, 1, 2024), D(15, 1, 2024), 0, 0, 0);
    findBook(1)->availableCopies = 1;
    std::string out = run(issueBook, "1\n1\n01-02-2024\n");
    EXPECT_TRUE(has(out, "This member already has this book issued."));
    EXPECT_EQ(countTrans(), 1);
}

TEST_F(IssueBookTest, CanBorrowAgainAfterReturning) {
    mkTrans(1, 1, 1, D(1, 1, 2024), D(15, 1, 2024), D(5, 1, 2024), 1, 0);
    std::string out = run(issueBook, "1\n1\n01-02-2024\n");
    EXPECT_TRUE(has(out, "Book issued."));
    EXPECT_EQ(countTrans(), 2);
}

TEST_F(IssueBookTest, DifferentMemberCanBorrowSameTitle) {
    mkMember(2, "Bob", "7654321", "b@x.com");
    mkTrans(1, 1, 1, D(1, 1, 2024), D(15, 1, 2024), 0, 0, 0);
    findBook(1)->availableCopies = 1;
    std::string out = run(issueBook, "1\n2\n01-02-2024\n");
    EXPECT_TRUE(has(out, "Book issued."));
    EXPECT_EQ(countTrans(), 2);
}

TEST_F(IssueBookTest, InvalidDateRejected) {
    std::string out = run(issueBook, "1\n1\n31-02-2024\n");
    EXPECT_TRUE(has(out, "Invalid date."));
    EXPECT_EQ(countTrans(), 0);
    EXPECT_EQ(findBook(1)->availableCopies, 2);
}

/* ============================ returnBook ============================ */

class ReturnBookTest : public LibraryTest {
protected:
    void SetUp() override {
        LibraryTest::SetUp();
        mkBook(1, "Dune", "Herbert", "SciFi", 3, 2);          /* 1 issued */
        mkMember(1, "Alice", "1234567", "a@x.com");
        mkTrans(1, 1, 1, D(1, 1, 2024), D(15, 1, 2024), 0, 0, 0.0f);
    }
};

TEST_F(ReturnBookTest, OnTimeReturnHasNoFine) {
    std::string out = run(returnBook, "1\n1\n10-01-2024\n");
    EXPECT_TRUE(has(out, "Book returned on time. No fine."));
    EXPECT_TRUE(has(out, "Copies available: 3"));
    EXPECT_EQ(transHead->returned, 1);
    EXPECT_EQ(transHead->returnDate, D(10, 1, 2024));
    EXPECT_FLOAT_EQ(transHead->fine, 0.0f);
    EXPECT_EQ(findBook(1)->availableCopies, 3);
}

TEST_F(ReturnBookTest, ReturnOnDueDateIsNotLate) {
    std::string out = run(returnBook, "1\n1\n15-01-2024\n");
    EXPECT_TRUE(has(out, "returned on time"));
    EXPECT_FLOAT_EQ(transHead->fine, 0.0f);
}

TEST_F(ReturnBookTest, LateReturnChargesFinePerDay) {
    std::string out = run(returnBook, "1\n1\n20-01-2024\n");
    EXPECT_TRUE(has(out, "returned LATE by 5 day(s). Fine: Rs. 10.00"));
    EXPECT_FLOAT_EQ(transHead->fine, 5 * FINE_PER_DAY);
    EXPECT_EQ(transHead->returned, 1);
}

TEST_F(ReturnBookTest, OneDayLate) {
    std::string out = run(returnBook, "1\n1\n16-01-2024\n");
    EXPECT_TRUE(has(out, "LATE by 1 day(s)"));
    EXPECT_FLOAT_EQ(transHead->fine, FINE_PER_DAY);
}

TEST_F(ReturnBookTest, BlankDateMeansToday) {
    run(returnBook, "1\n1\n\n");
    EXPECT_EQ(transHead->returnDate, today());
    EXPECT_EQ(transHead->returned, 1);
}

TEST_F(ReturnBookTest, PersistsToFiles) {
    run(returnBook, "1\n1\n20-01-2024\n");
    EXPECT_EQ(readFile(TRANS_FILE),
              "1|1|1|01-01-2024|15-01-2024|20-01-2024|1|10.00\n");
    EXPECT_EQ(readFile(BOOK_FILE), "1|Dune|Herbert|SciFi|3|3\n");
}

TEST_F(ReturnBookTest, InvalidBookId) {
    EXPECT_TRUE(has(run(returnBook, "abc\n"), "Invalid Book ID."));
}

TEST_F(ReturnBookTest, InvalidMemberId) {
    EXPECT_TRUE(has(run(returnBook, "1\nabc\n"), "Invalid Member ID."));
}

TEST_F(ReturnBookTest, BookNotFound) {
    EXPECT_TRUE(has(run(returnBook, "99\n1\n"), "Book not found."));
}

TEST_F(ReturnBookTest, MemberNotFound) {
    EXPECT_TRUE(has(run(returnBook, "1\n99\n"), "Member not found."));
}

TEST_F(ReturnBookTest, NoActiveIssueRecord) {
    mkMember(2, "Bob", "7654321", "b@x.com");
    std::string out = run(returnBook, "1\n2\n");
    EXPECT_TRUE(has(out, "No active issue record for this book and member."));
}

TEST_F(ReturnBookTest, AlreadyReturnedIsNotActive) {
    transHead->returned = 1;
    EXPECT_TRUE(has(run(returnBook, "1\n1\n"), "No active issue record"));
}

TEST_F(ReturnBookTest, InvalidDate) {
    std::string out = run(returnBook, "1\n1\nnot-a-date\n");
    EXPECT_TRUE(has(out, "Invalid date."));
    EXPECT_EQ(transHead->returned, 0);
}

TEST_F(ReturnBookTest, ReturnBeforeIssueDateRejected) {
    std::string out = run(returnBook, "1\n1\n31-12-2023\n");
    EXPECT_TRUE(has(out, "Return date cannot be before the issue date."));
    EXPECT_EQ(transHead->returned, 0);
    EXPECT_EQ(findBook(1)->availableCopies, 2);
}

/* ========================== viewTransactions ======================== */

class ViewTransactionsTest : public LibraryTest {};

TEST_F(ViewTransactionsTest, Empty) {
    EXPECT_TRUE(has(run(viewTransactions), "No transactions recorded."));
}

TEST_F(ViewTransactionsTest, ShowsHeaderAndRows) {
    mkTrans(1, 2, 3, D(1, 1, 2024), D(15, 1, 2024), 0, 0, 0.0f);
    mkTrans(2, 4, 5, D(1, 1, 2024), D(15, 1, 2024), D(20, 1, 2024), 1, 10.0f);
    std::string out = run(viewTransactions);
    EXPECT_TRUE(has(out, "TID"));
    EXPECT_TRUE(has(out, "Status"));
    EXPECT_TRUE(has(out, "Issued"));
    EXPECT_TRUE(has(out, "Returned"));
    EXPECT_TRUE(has(out, "01-01-2024"));
    EXPECT_TRUE(has(out, "15-01-2024"));
    EXPECT_TRUE(has(out, "20-01-2024"));
    EXPECT_TRUE(has(out, "10.00"));
}

TEST_F(ViewTransactionsTest, UnreturnedShowsDashForReturnDate) {
    mkTrans(1, 2, 3, D(1, 1, 2024), D(15, 1, 2024), 0, 0, 0.0f);
    std::string out = run(viewTransactions);
    EXPECT_TRUE(has(out, " - "));
}

/* ========================== memberFineDetails ======================= */

class MemberFineTest : public LibraryTest {
protected:
    void SetUp() override {
        LibraryTest::SetUp();
        mkMember(1, "Alice", "1234567", "a@x.com");
        mkMember(2, "Bob", "7654321", "b@x.com");
    }
};

TEST_F(MemberFineTest, InvalidId) {
    EXPECT_TRUE(has(run(memberFineDetails, "abc\n"), "Invalid Member ID."));
}

TEST_F(MemberFineTest, MemberNotFound) {
    EXPECT_TRUE(has(run(memberFineDetails, "99\n"), "Member not found."));
}

TEST_F(MemberFineTest, NoTransactions) {
    std::string out = run(memberFineDetails, "1\n");
    EXPECT_TRUE(has(out, "Fine details for Alice (ID 1)"));
    EXPECT_TRUE(has(out, "No transactions for this member."));
}

TEST_F(MemberFineTest, SumsFinesOfReturnedBooks) {
    mkTrans(1, 1, 1, D(1, 1, 2024), D(15, 1, 2024), D(20, 1, 2024), 1, 10.0f);
    mkTrans(2, 2, 1, D(1, 1, 2024), D(15, 1, 2024), D(18, 1, 2024), 1, 6.0f);
    std::string out = run(memberFineDetails, "1\n");
    EXPECT_TRUE(has(out, "Total fine (incl. accruing): Rs. 16.00"));
    EXPECT_FALSE(has(out, "(accruing)"));
}

TEST_F(MemberFineTest, OverdueUnreturnedBookAccruesFine) {
    time_t due = D(1, 1, 2020);
    mkTrans(1, 1, 1, D(18, 12, 2019), due, 0, 0, 0.0f);
    float expected = daysBetween(due, today()) * FINE_PER_DAY;
    std::string out = run(memberFineDetails, "1\n");
    EXPECT_TRUE(has(out, "(accruing)"));
    EXPECT_TRUE(has(out, "Total fine (incl. accruing): Rs. " + money(expected)));
}

TEST_F(MemberFineTest, NotYetDueUnreturnedBookHasZeroFine) {
    time_t due = today() + (time_t)10 * 86400;
    mkTrans(1, 1, 1, today(), due, 0, 0, 0.0f);
    std::string out = run(memberFineDetails, "1\n");
    EXPECT_TRUE(has(out, "(accruing)"));
    EXPECT_TRUE(has(out, "Total fine (incl. accruing): Rs. 0.00"));
}

TEST_F(MemberFineTest, MixesReturnedAndAccruing) {
    time_t due = D(1, 1, 2020);
    mkTrans(1, 1, 1, D(1, 1, 2024), D(15, 1, 2024), D(20, 1, 2024), 1, 10.0f);
    mkTrans(2, 2, 1, D(18, 12, 2019), due, 0, 0, 0.0f);
    float expected = 10.0f + daysBetween(due, today()) * FINE_PER_DAY;
    std::string out = run(memberFineDetails, "1\n");
    EXPECT_TRUE(has(out, "Rs. " + money(expected)));
}

TEST_F(MemberFineTest, IgnoresOtherMembersTransactions) {
    mkTrans(1, 1, 2, D(1, 1, 2024), D(15, 1, 2024), D(30, 1, 2024), 1, 30.0f);
    std::string out = run(memberFineDetails, "1\n");
    EXPECT_TRUE(has(out, "No transactions for this member."));
    EXPECT_FALSE(has(out, "30.00"));
}
