/*
 * test_library.cpp - single-file GoogleTest suite for the Library Management System
 *
 * Sections:  1. Shared helpers & fixture
 *            2. utils.c        3. storage.c     4. book.c
 *            5. member.c       6. transaction.c
 *
 * Build:  make test        (or:  g++ -std=c++17 -Iinclude -o t tests/test_library.cpp
 *                                build/<objects>.o -lgtest_main -lgtest -pthread)
 */

/* ===================== 1. SHARED HELPERS & FIXTURE ===================== */
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

extern "C" {
#include "library.h"
}

namespace fs = std::filesystem;

/* ------------------------------------------------------------------ */
/* stdin / stdout helpers                                              */
/* ------------------------------------------------------------------ */

/* Makes the next reads from stdin come from 'text'. */
inline void setInput(const std::string &text) {
    static int counter = 0;
    std::string name = "stdin_input_" + std::to_string(counter++) + ".txt";
    std::ofstream(name, std::ios::binary) << text;
    ASSERT_NE(std::freopen(name.c_str(), "r", stdin), nullptr);
}

/* Feeds 'input' to stdin, runs fn() and returns everything it printed. */
inline std::string run(void (*fn)(void), const std::string &input = "") {
    setInput(input);
    testing::internal::CaptureStdout();
    fn();
    return testing::internal::GetCapturedStdout();
}

inline bool has(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
}

inline std::string readFile(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

inline void writeFile(const std::string &path, const std::string &text) {
    std::ofstream(path, std::ios::binary) << text;
}

/* ------------------------------------------------------------------ */
/* Date helper: local midnight of d-m-y                                */
/* ------------------------------------------------------------------ */
inline time_t D(int d, int m, int y) {
    struct tm t = {};
    t.tm_mday = d;
    t.tm_mon = m - 1;
    t.tm_year = y - 1900;
    t.tm_isdst = -1;
    return mktime(&t);
}

/* ------------------------------------------------------------------ */
/* Record builders (nodes are malloc'ed so freeAll() can release them) */
/* ------------------------------------------------------------------ */
inline Book *mkBook(int id, const char *title, const char *author,
                    const char *cat, int total, int avail) {
    Book *b = (Book *)calloc(1, sizeof(Book));
    b->id = id;
    snprintf(b->title, sizeof b->title, "%s", title);
    snprintf(b->author, sizeof b->author, "%s", author);
    snprintf(b->category, sizeof b->category, "%s", cat);
    b->totalCopies = total;
    b->availableCopies = avail;
    appendBook(b);
    return b;
}

inline Member *mkMember(int id, const char *name, const char *phone,
                        const char *email) {
    Member *m = (Member *)calloc(1, sizeof(Member));
    m->id = id;
    snprintf(m->name, sizeof m->name, "%s", name);
    snprintf(m->phone, sizeof m->phone, "%s", phone);
    snprintf(m->email, sizeof m->email, "%s", email);
    appendMember(m);
    return m;
}

inline Transaction *mkTrans(int id, int bookId, int memberId, time_t issue,
                            time_t due, time_t ret, int returned,
                            float fine) {
    Transaction *t = (Transaction *)calloc(1, sizeof(Transaction));
    t->transId = id;
    t->bookId = bookId;
    t->memberId = memberId;
    t->issueDate = issue;
    t->expectedReturn = due;
    t->returnDate = ret;
    t->returned = returned;
    t->fine = fine;
    appendTransaction(t);
    return t;
}

inline int countBooks() {
    int n = 0;
    for (Book *p = bookHead; p; p = p->next) n++;
    return n;
}
inline int countMembers() {
    int n = 0;
    for (Member *p = memberHead; p; p = p->next) n++;
    return n;
}
inline int countTrans() {
    int n = 0;
    for (Transaction *p = transHead; p; p = p->next) n++;
    return n;
}

/* ------------------------------------------------------------------ */
/* Base fixture: empty lists + private working directory               */
/* ------------------------------------------------------------------ */
class LibraryTest : public ::testing::Test {
protected:
    fs::path oldCwd_;
    fs::path work_;

    void SetUp() override {
        std::error_code ec;
        oldCwd_ = fs::current_path();
        work_ = fs::temp_directory_path() / "lms_gtest_workdir";
        fs::remove_all(work_, ec);
        fs::create_directories(work_);
        fs::current_path(work_);
        freeAll();
    }

    void TearDown() override {
        std::error_code ec;
        freeAll();
        fs::current_path(oldCwd_);
        fs::remove_all(work_, ec);
    }
};

/* ===================== TESTS FOR utils.c ===================== */
class UtilsTest : public LibraryTest {};

/* ============================ readLine ============================== */

TEST_F(UtilsTest, ReadLine_ReadsLineAndStripsNewline) {
    setInput("hello world\n");
    char buf[32];
    testing::internal::CaptureStdout();
    readLine("Name: ", buf, sizeof buf);
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out, "Name: ");
    EXPECT_STREQ(buf, "hello world");
}

TEST_F(UtilsTest, ReadLine_EmptyLineGivesEmptyString) {
    setInput("\n");
    char buf[8] = "xxx";
    testing::internal::CaptureStdout();
    readLine("> ", buf, sizeof buf);
    testing::internal::GetCapturedStdout();
    EXPECT_STREQ(buf, "");
}

TEST_F(UtilsTest, ReadLine_TruncatesToBufferSize) {
    setInput("abcdefgh\n");
    char buf[4];
    testing::internal::CaptureStdout();
    readLine("> ", buf, sizeof buf);
    testing::internal::GetCapturedStdout();
    EXPECT_STREQ(buf, "abc");
}

TEST_F(UtilsTest, ReadLine_EofGivesEmptyString) {
    setInput("");
    char buf[8] = "xxx";
    testing::internal::CaptureStdout();
    readLine("> ", buf, sizeof buf);
    testing::internal::GetCapturedStdout();
    EXPECT_STREQ(buf, "");
}

TEST_F(UtilsTest, ReadLine_LastLineWithoutNewline) {
    setInput("tail");
    char buf[16];
    testing::internal::CaptureStdout();
    readLine("> ", buf, sizeof buf);
    testing::internal::GetCapturedStdout();
    EXPECT_STREQ(buf, "tail");
}

/* ============================= readInt ============================== */

TEST_F(UtilsTest, ReadInt_ValidPositive) {
    setInput("42\n");
    int v = 0;
    testing::internal::CaptureStdout();
    EXPECT_EQ(readInt("n: ", &v), 1);
    testing::internal::GetCapturedStdout();
    EXPECT_EQ(v, 42);
}

TEST_F(UtilsTest, ReadInt_ValidNegative) {
    setInput("-7\n");
    int v = 0;
    testing::internal::CaptureStdout();
    EXPECT_EQ(readInt("n: ", &v), 1);
    testing::internal::GetCapturedStdout();
    EXPECT_EQ(v, -7);
}

TEST_F(UtilsTest, ReadInt_Zero) {
    setInput("0\n");
    int v = 99;
    testing::internal::CaptureStdout();
    EXPECT_EQ(readInt("n: ", &v), 1);
    testing::internal::GetCapturedStdout();
    EXPECT_EQ(v, 0);
}

TEST_F(UtilsTest, ReadInt_RejectsLetters) {
    setInput("abc\n");
    int v = 5;
    testing::internal::CaptureStdout();
    EXPECT_EQ(readInt("n: ", &v), 0);
    testing::internal::GetCapturedStdout();
    EXPECT_EQ(v, 5);   /* output untouched on failure */
}

TEST_F(UtilsTest, ReadInt_RejectsTrailingGarbage) {
    setInput("12abc\n");
    int v = 5;
    testing::internal::CaptureStdout();
    EXPECT_EQ(readInt("n: ", &v), 0);
    testing::internal::GetCapturedStdout();
}

TEST_F(UtilsTest, ReadInt_RejectsEmptyInput) {
    setInput("\n");
    int v = 5;
    testing::internal::CaptureStdout();
    EXPECT_EQ(readInt("n: ", &v), 0);
    testing::internal::GetCapturedStdout();
}

TEST_F(UtilsTest, ReadInt_RejectsDecimal) {
    setInput("3.5\n");
    int v = 0;
    testing::internal::CaptureStdout();
    EXPECT_EQ(readInt("n: ", &v), 0);
    testing::internal::GetCapturedStdout();
}

/* ============================= copyStr ============================== */

TEST_F(UtilsTest, CopyStr_NormalCopy) {
    char dst[16];
    copyStr(dst, "hello", sizeof dst);
    EXPECT_STREQ(dst, "hello");
}

TEST_F(UtilsTest, CopyStr_TruncatesAndTerminates) {
    char dst[4];
    memset(dst, 'x', sizeof dst);
    copyStr(dst, "abcdef", sizeof dst);
    EXPECT_STREQ(dst, "abc");
}

TEST_F(UtilsTest, CopyStr_ExactFit) {
    char dst[6];
    copyStr(dst, "hello", sizeof dst);
    EXPECT_STREQ(dst, "hello");
}

TEST_F(UtilsTest, CopyStr_EmptySource) {
    char dst[4] = "abc";
    copyStr(dst, "", sizeof dst);
    EXPECT_STREQ(dst, "");
}

TEST_F(UtilsTest, CopyStr_SizeOneGivesEmptyString) {
    char dst[1] = {'x'};
    copyStr(dst, "abc", 1);
    EXPECT_EQ(dst[0], '\0');
}

/* ============================== isBlank ============================= */

TEST_F(UtilsTest, IsBlank_EmptyString)        { EXPECT_EQ(isBlank(""), 1); }
TEST_F(UtilsTest, IsBlank_OnlyWhitespace)     { EXPECT_EQ(isBlank(" \t\r\n "), 1); }
TEST_F(UtilsTest, IsBlank_TextIsNotBlank)     { EXPECT_EQ(isBlank("a"), 0); }
TEST_F(UtilsTest, IsBlank_PaddedTextNotBlank) { EXPECT_EQ(isBlank("  x  "), 0); }

/* ======================== containsIgnoreCase ======================== */

TEST_F(UtilsTest, Contains_ExactMatch)   { EXPECT_EQ(containsIgnoreCase("Hello", "Hello"), 1); }
TEST_F(UtilsTest, Contains_IgnoresCase)  { EXPECT_EQ(containsIgnoreCase("The Great Gatsby", "great"), 1); }
TEST_F(UtilsTest, Contains_UpperPattern) { EXPECT_EQ(containsIgnoreCase("gatsby", "GATSBY"), 1); }
TEST_F(UtilsTest, Contains_Substring)    { EXPECT_EQ(containsIgnoreCase("abcdef", "cde"), 1); }
TEST_F(UtilsTest, Contains_AtEnd)        { EXPECT_EQ(containsIgnoreCase("abcdef", "ef"), 1); }
TEST_F(UtilsTest, Contains_NoMatch)      { EXPECT_EQ(containsIgnoreCase("abcdef", "xyz"), 0); }
TEST_F(UtilsTest, Contains_EmptyPatternAlwaysMatches) {
    EXPECT_EQ(containsIgnoreCase("abc", ""), 1);
    EXPECT_EQ(containsIgnoreCase("", ""), 1);
}
TEST_F(UtilsTest, Contains_PatternLongerThanText) { EXPECT_EQ(containsIgnoreCase("ab", "abc"), 0); }
TEST_F(UtilsTest, Contains_EmptyTextNonEmptyPattern) { EXPECT_EQ(containsIgnoreCase("", "a"), 0); }

/* ============================= validPhone =========================== */

TEST_F(UtilsTest, Phone_TenDigits)            { EXPECT_EQ(validPhone("9876543210"), 1); }
TEST_F(UtilsTest, Phone_WithPlusSpaceDash)    { EXPECT_EQ(validPhone("+91 98765-43210"), 1); }
TEST_F(UtilsTest, Phone_MinBoundarySevenDigits)  { EXPECT_EQ(validPhone("1234567"), 1); }
TEST_F(UtilsTest, Phone_MaxBoundaryFifteenDigits){ EXPECT_EQ(validPhone("123456789012345"), 1); }
TEST_F(UtilsTest, Phone_TooShort)             { EXPECT_EQ(validPhone("123456"), 0); }
TEST_F(UtilsTest, Phone_TooLong)              { EXPECT_EQ(validPhone("1234567890123456"), 0); }
TEST_F(UtilsTest, Phone_LettersRejected)      { EXPECT_EQ(validPhone("98a76543210"), 0); }
TEST_F(UtilsTest, Phone_EmptyRejected)        { EXPECT_EQ(validPhone(""), 0); }
TEST_F(UtilsTest, Phone_SymbolsAloneRejected) { EXPECT_EQ(validPhone("+-+- "), 0); }
TEST_F(UtilsTest, Phone_ParenthesesRejected)  { EXPECT_EQ(validPhone("(044)1234567"), 0); }

/* ============================= validEmail =========================== */

TEST_F(UtilsTest, Email_Valid)                { EXPECT_EQ(validEmail("a@b.com"), 1); }
TEST_F(UtilsTest, Email_ValidShortTld)        { EXPECT_EQ(validEmail("a@b.c"), 1); }
TEST_F(UtilsTest, Email_MissingAt)            { EXPECT_EQ(validEmail("ab.com"), 0); }
TEST_F(UtilsTest, Email_NothingBeforeAt)      { EXPECT_EQ(validEmail("@b.com"), 0); }
TEST_F(UtilsTest, Email_MissingDotAfterAt)    { EXPECT_EQ(validEmail("a@bcom"), 0); }
TEST_F(UtilsTest, Email_DotImmediatelyAfterAt){ EXPECT_EQ(validEmail("a@.com"), 0); }
TEST_F(UtilsTest, Email_DotLastChar)          { EXPECT_EQ(validEmail("a@b."), 0); }
TEST_F(UtilsTest, Email_ContainsSpace)        { EXPECT_EQ(validEmail("a b@c.com"), 0); }
TEST_F(UtilsTest, Email_Empty)                { EXPECT_EQ(validEmail(""), 0); }
TEST_F(UtilsTest, Email_DotOnlyBeforeAtRejected) { EXPECT_EQ(validEmail("a.b@c"), 0); }

/* =============================== today ============================== */

TEST_F(UtilsTest, Today_IsLocalMidnight) {
    time_t t = today();
    struct tm tm = *localtime(&t);
    EXPECT_EQ(tm.tm_hour, 0);
    EXPECT_EQ(tm.tm_min, 0);
    EXPECT_EQ(tm.tm_sec, 0);
}

TEST_F(UtilsTest, Today_IsWithinLastDay) {
    time_t now = time(NULL);
    time_t t = today();
    EXPECT_LE(t, now);
    EXPECT_GT(t, now - 25 * 3600);
}

/* ============================= parseDate ============================ */

TEST_F(UtilsTest, ParseDate_ValidDate) {
    time_t t = 0;
    ASSERT_EQ(parseDate("15-08-2024", &t), 1);
    struct tm tm = *localtime(&t);
    EXPECT_EQ(tm.tm_mday, 15);
    EXPECT_EQ(tm.tm_mon, 7);
    EXPECT_EQ(tm.tm_year, 124);
}

TEST_F(UtilsTest, ParseDate_NoLeadingZeros) {
    time_t t = 0;
    ASSERT_EQ(parseDate("5-3-2024", &t), 1);
    EXPECT_EQ(t, D(5, 3, 2024));
}

TEST_F(UtilsTest, ParseDate_LeapDayValid) {
    time_t t = 0;
    EXPECT_EQ(parseDate("29-02-2024", &t), 1);
}

TEST_F(UtilsTest, ParseDate_LeapDayInvalidYear) {
    time_t t = 0;
    EXPECT_EQ(parseDate("29-02-2023", &t), 0);
}

TEST_F(UtilsTest, ParseDate_Feb31Rejected) {
    time_t t = 0;
    EXPECT_EQ(parseDate("31-02-2024", &t), 0);
}

TEST_F(UtilsTest, ParseDate_31stOf30DayMonthRejected) {
    time_t t = 0;
    EXPECT_EQ(parseDate("31-04-2024", &t), 0);
}

TEST_F(UtilsTest, ParseDate_DayZeroRejected) {
    time_t t = 0;
    EXPECT_EQ(parseDate("00-01-2024", &t), 0);
}

TEST_F(UtilsTest, ParseDate_Day32Rejected) {
    time_t t = 0;
    EXPECT_EQ(parseDate("32-01-2024", &t), 0);
}

TEST_F(UtilsTest, ParseDate_MonthOutOfRange) {
    time_t t = 0;
    EXPECT_EQ(parseDate("10-13-2024", &t), 0);
    EXPECT_EQ(parseDate("10-00-2024", &t), 0);
}

TEST_F(UtilsTest, ParseDate_YearOutOfRange) {
    time_t t = 0;
    EXPECT_EQ(parseDate("10-10-1899", &t), 0);
    EXPECT_EQ(parseDate("10-10-2101", &t), 0);
}

TEST_F(UtilsTest, ParseDate_GarbageRejected) {
    time_t t = 0;
    EXPECT_EQ(parseDate("garbage", &t), 0);
    EXPECT_EQ(parseDate("-", &t), 0);
    EXPECT_EQ(parseDate("", &t), 0);
    EXPECT_EQ(parseDate("12-12", &t), 0);
}

TEST_F(UtilsTest, ParseDate_OutputUntouchedOnFailure) {
    time_t t = 1234;
    EXPECT_EQ(parseDate("31-02-2024", &t), 0);
    EXPECT_EQ(t, 1234);
}

/* ============================= readDate ============================= */

TEST_F(UtilsTest, ReadDate_BlankMeansToday) {
    setInput("\n");
    time_t t = 0;
    testing::internal::CaptureStdout();
    EXPECT_EQ(readDate("Date: ", &t), 1);
    testing::internal::GetCapturedStdout();
    EXPECT_EQ(t, today());
}

TEST_F(UtilsTest, ReadDate_ValidDate) {
    setInput("01-02-2024\n");
    time_t t = 0;
    testing::internal::CaptureStdout();
    EXPECT_EQ(readDate("Date: ", &t), 1);
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out, "Date: ");
    EXPECT_EQ(t, D(1, 2, 2024));
}

TEST_F(UtilsTest, ReadDate_InvalidDate) {
    setInput("99-99-9999\n");
    time_t t = 0;
    testing::internal::CaptureStdout();
    EXPECT_EQ(readDate("Date: ", &t), 0);
    testing::internal::GetCapturedStdout();
}

/* ============================= fmtDate ============================== */

TEST_F(UtilsTest, FmtDate_ZeroIsDash) {
    char buf[16];
    fmtDate(0, buf, sizeof buf);
    EXPECT_STREQ(buf, "-");
}

TEST_F(UtilsTest, FmtDate_FormatsDDMMYYYY) {
    char buf[16];
    fmtDate(D(5, 3, 2024), buf, sizeof buf);
    EXPECT_STREQ(buf, "05-03-2024");
}

TEST_F(UtilsTest, FmtDate_RoundTripsWithParseDate) {
    char buf[16];
    time_t back = 0;
    fmtDate(D(31, 12, 2023), buf, sizeof buf);
    ASSERT_EQ(parseDate(buf, &back), 1);
    EXPECT_EQ(back, D(31, 12, 2023));
}

/* ============================ daysBetween =========================== */

TEST_F(UtilsTest, DaysBetween_SameDayIsZero) {
    EXPECT_EQ(daysBetween(D(1, 1, 2024), D(1, 1, 2024)), 0);
}

TEST_F(UtilsTest, DaysBetween_Forward) {
    EXPECT_EQ(daysBetween(D(1, 1, 2024), D(15, 1, 2024)), 14);
}

TEST_F(UtilsTest, DaysBetween_Backward) {
    EXPECT_EQ(daysBetween(D(15, 1, 2024), D(1, 1, 2024)), -14);
}

TEST_F(UtilsTest, DaysBetween_AcrossMonthBoundaryLeapYear) {
    EXPECT_EQ(daysBetween(D(28, 2, 2024), D(1, 3, 2024)), 2);
}

TEST_F(UtilsTest, DaysBetween_RoundsAcrossDstChange) {
    /* Whatever the local DST rules are, whole calendar days must come out whole. */
    EXPECT_EQ(daysBetween(D(1, 3, 2024), D(1, 4, 2024)), 31);
    EXPECT_EQ(daysBetween(D(1, 10, 2024), D(1, 11, 2024)), 31);
}

TEST_F(UtilsTest, DaysBetween_FullYear) {
    EXPECT_EQ(daysBetween(D(1, 1, 2023), D(1, 1, 2024)), 365);
}

/* ===================== TESTS FOR storage.c ===================== */
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

/* ===================== TESTS FOR book.c ===================== */
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

/* ===================== TESTS FOR member.c ===================== */
class MemberTest : public LibraryTest {};

/* =========================== registerMember ========================= */

TEST_F(MemberTest, Register_ValidInputAddsMemberAndSaves) {
    std::string out = run(registerMember, "1\nJohn Doe\n9876543210\njohn@x.com\n");
    EXPECT_TRUE(has(out, "Member registered successfully."));
    ASSERT_EQ(countMembers(), 1);
    EXPECT_EQ(memberHead->id, 1);
    EXPECT_STREQ(memberHead->name, "John Doe");
    EXPECT_STREQ(memberHead->phone, "9876543210");
    EXPECT_STREQ(memberHead->email, "john@x.com");
    EXPECT_EQ(readFile(MEMBER_FILE), "1|John Doe|9876543210|john@x.com\n");
}

TEST_F(MemberTest, Register_AppendsToEnd) {
    mkMember(1, "A", "1234567", "a@b.com");
    run(registerMember, "2\nB\n1234567\nb@b.com\n");
    ASSERT_EQ(countMembers(), 2);
    EXPECT_EQ(memberHead->next->id, 2);
}

TEST_F(MemberTest, Register_RejectsNonNumericOrNonPositiveId) {
    EXPECT_TRUE(has(run(registerMember, "abc\n"), "Invalid Member ID"));
    EXPECT_TRUE(has(run(registerMember, "0\n"), "Invalid Member ID"));
    EXPECT_TRUE(has(run(registerMember, "-1\n"), "Invalid Member ID"));
    EXPECT_EQ(countMembers(), 0);
}

TEST_F(MemberTest, Register_RejectsDuplicateId) {
    mkMember(1, "A", "1234567", "a@b.com");
    std::string out = run(registerMember, "1\nB\n1234567\nb@b.com\n");
    EXPECT_TRUE(has(out, "Member ID 1 already exists"));
    EXPECT_EQ(countMembers(), 1);
}

TEST_F(MemberTest, Register_RejectsBlankName) {
    std::string out = run(registerMember, "1\n  \n1234567\na@b.com\n");
    EXPECT_TRUE(has(out, "Name cannot be empty."));
    EXPECT_EQ(countMembers(), 0);
}

TEST_F(MemberTest, Register_RejectsInvalidPhone) {
    std::string out = run(registerMember, "1\nA\n12\na@b.com\n");
    EXPECT_TRUE(has(out, "Invalid phone number."));
    EXPECT_EQ(countMembers(), 0);
}

TEST_F(MemberTest, Register_RejectsInvalidEmail) {
    std::string out = run(registerMember, "1\nA\n1234567\nnot-an-email\n");
    EXPECT_TRUE(has(out, "Invalid email address."));
    EXPECT_EQ(countMembers(), 0);
}

/* ============================ viewMembers =========================== */

TEST_F(MemberTest, ViewMembers_Empty) {
    EXPECT_TRUE(has(run(viewMembers), "No members registered."));
}

TEST_F(MemberTest, ViewMembers_ListsHeaderAndRows) {
    mkMember(1, "Alice", "1111111", "alice@x.com");
    mkMember(2, "Bob", "2222222", "bob@x.com");
    std::string out = run(viewMembers);
    EXPECT_TRUE(has(out, "Name"));
    EXPECT_TRUE(has(out, "Email"));
    EXPECT_TRUE(has(out, "Alice"));
    EXPECT_TRUE(has(out, "bob@x.com"));
}

TEST_F(MemberTest, ViewMembers_TruncatesLongNameTo25Chars) {
    std::string name = std::string(25, 'n') + "QQQ";
    mkMember(1, name.c_str(), "1111111", "a@b.com");
    std::string out = run(viewMembers);
    EXPECT_TRUE(has(out, std::string(25, 'n')));
    EXPECT_FALSE(has(out, "QQQ"));
}

/* ============================ searchMember ========================== */

class SearchMemberTest : public MemberTest {
protected:
    void SetUp() override {
        MemberTest::SetUp();
        mkMember(1, "Alice Smith", "1111111", "alice@x.com");
        mkMember(2, "Bob Smith", "2222222", "bob@x.com");
        mkMember(3, "Carol", "3333333", "carol@x.com");
    }
};

TEST_F(SearchMemberTest, InvalidChoice) {
    EXPECT_TRUE(has(run(searchMember, "3\n"), "Invalid choice."));
    EXPECT_TRUE(has(run(searchMember, "z\n"), "Invalid choice."));
}

TEST_F(SearchMemberTest, ById_Found) {
    std::string out = run(searchMember, "1\n2\n");
    EXPECT_TRUE(has(out, "Bob Smith"));
    EXPECT_FALSE(has(out, "Alice"));
}

TEST_F(SearchMemberTest, ById_NotFound) {
    EXPECT_TRUE(has(run(searchMember, "1\n99\n"), "No matching members found."));
}

TEST_F(SearchMemberTest, ById_InvalidNumber) {
    EXPECT_TRUE(has(run(searchMember, "1\nabc\n"), "Invalid ID."));
}

TEST_F(SearchMemberTest, ByName_CaseInsensitiveMultipleHits) {
    std::string out = run(searchMember, "2\nSMITH\n");
    EXPECT_TRUE(has(out, "Alice Smith"));
    EXPECT_TRUE(has(out, "Bob Smith"));
    EXPECT_FALSE(has(out, "Carol"));
}

TEST_F(SearchMemberTest, ByName_NoMatch) {
    EXPECT_TRUE(has(run(searchMember, "2\nzzz\n"), "No matching members found."));
}

TEST_F(SearchMemberTest, ByName_BlankText) {
    EXPECT_TRUE(has(run(searchMember, "2\n  \n"), "Search text cannot be empty."));
}

/* ============================ updateMember ========================== */

TEST_F(MemberTest, Update_InvalidId) {
    EXPECT_TRUE(has(run(updateMember, "x\n"), "Invalid Member ID."));
}

TEST_F(MemberTest, Update_NotFound) {
    EXPECT_TRUE(has(run(updateMember, "5\n"), "Member not found."));
}

TEST_F(MemberTest, Update_BlankAnswersKeepValues) {
    mkMember(1, "Old", "1234567", "old@x.com");
    std::string out = run(updateMember, "1\n\n\n\n");
    EXPECT_TRUE(has(out, "Member updated successfully."));
    EXPECT_STREQ(memberHead->name, "Old");
    EXPECT_STREQ(memberHead->phone, "1234567");
    EXPECT_STREQ(memberHead->email, "old@x.com");
}

TEST_F(MemberTest, Update_ChangesAllFieldsAndSaves) {
    mkMember(1, "Old", "1234567", "old@x.com");
    run(updateMember, "1\nNew Name\n9999999\nnew@x.com\n");
    EXPECT_STREQ(memberHead->name, "New Name");
    EXPECT_STREQ(memberHead->phone, "9999999");
    EXPECT_STREQ(memberHead->email, "new@x.com");
    EXPECT_EQ(readFile(MEMBER_FILE), "1|New Name|9999999|new@x.com\n");
}

TEST_F(MemberTest, Update_InvalidPhoneNotChanged) {
    mkMember(1, "Old", "1234567", "old@x.com");
    std::string out = run(updateMember, "1\n\nabc\n\n");
    EXPECT_TRUE(has(out, "Invalid phone; not changed."));
    EXPECT_STREQ(memberHead->phone, "1234567");
}

TEST_F(MemberTest, Update_InvalidEmailNotChanged) {
    mkMember(1, "Old", "1234567", "old@x.com");
    std::string out = run(updateMember, "1\n\n\nbad-email\n");
    EXPECT_TRUE(has(out, "Invalid email; not changed."));
    EXPECT_STREQ(memberHead->email, "old@x.com");
}

/* ============================ deleteMember ========================== */

TEST_F(MemberTest, Delete_InvalidId) {
    EXPECT_TRUE(has(run(deleteMember, "q\n"), "Invalid Member ID."));
}

TEST_F(MemberTest, Delete_NotFound) {
    EXPECT_TRUE(has(run(deleteMember, "4\n"), "Member not found."));
}

TEST_F(MemberTest, Delete_BlockedByOpenLoan) {
    mkMember(1, "A", "1234567", "a@b.com");
    mkTrans(1, 1, 1, D(1, 1, 2024), D(15, 1, 2024), 0, 0, 0);
    std::string out = run(deleteMember, "1\n");
    EXPECT_TRUE(has(out, "Cannot delete: member still has books issued."));
    EXPECT_EQ(countMembers(), 1);
}

TEST_F(MemberTest, Delete_AllowedWhenAllReturned) {
    mkMember(1, "A", "1234567", "a@b.com");
    mkTrans(1, 1, 1, D(1, 1, 2024), D(15, 1, 2024), D(5, 1, 2024), 1, 0);
    EXPECT_TRUE(has(run(deleteMember, "1\n"), "Member deleted successfully."));
    EXPECT_EQ(countMembers(), 0);
}

TEST_F(MemberTest, Delete_Head) {
    mkMember(1, "A", "1234567", "a@b.com");
    Member *b = mkMember(2, "B", "1234567", "b@b.com");
    run(deleteMember, "1\n");
    EXPECT_EQ(memberHead, b);
}

TEST_F(MemberTest, Delete_Middle) {
    Member *a = mkMember(1, "A", "1234567", "a@b.com");
    mkMember(2, "B", "1234567", "b@b.com");
    Member *c = mkMember(3, "C", "1234567", "c@b.com");
    run(deleteMember, "2\n");
    EXPECT_EQ(a->next, c);
    EXPECT_EQ(findMember(2), nullptr);
}

TEST_F(MemberTest, Delete_Tail) {
    mkMember(1, "A", "1234567", "a@b.com");
    Member *b = mkMember(2, "B", "1234567", "b@b.com");
    mkMember(3, "C", "1234567", "c@b.com");
    run(deleteMember, "3\n");
    EXPECT_EQ(b->next, nullptr);
}

TEST_F(MemberTest, Delete_SavesFile) {
    mkMember(1, "A", "1234567", "a@b.com");
    run(deleteMember, "1\n");
    EXPECT_EQ(readFile(MEMBER_FILE), "");
}

/* ============================= memberMenu =========================== */

TEST_F(MemberTest, Menu_ExitsOnZero) {
    EXPECT_TRUE(has(run(memberMenu, "0\n"), "MEMBER MANAGEMENT"));
}

TEST_F(MemberTest, Menu_InvalidTextThenExit) {
    EXPECT_TRUE(has(run(memberMenu, "abc\n0\n"), "Invalid input."));
}

TEST_F(MemberTest, Menu_UnknownNumberThenExit) {
    EXPECT_TRUE(has(run(memberMenu, "42\n0\n"), "Invalid choice."));
}

TEST_F(MemberTest, Menu_RegisterAndView) {
    std::string out = run(memberMenu, "1\n1\nZed\n1234567\nzed@x.com\n2\n0\n");
    EXPECT_TRUE(has(out, "Member registered successfully."));
    EXPECT_TRUE(has(out, "Zed"));
}

TEST_F(MemberTest, Menu_SearchOption) {
    mkMember(1, "Alice", "1111111", "a@x.com");
    EXPECT_TRUE(has(run(memberMenu, "3\n2\nali\n0\n"), "Alice"));
}

TEST_F(MemberTest, Menu_UpdateOption) {
    mkMember(1, "Old", "1111111", "a@x.com");
    run(memberMenu, "4\n1\nNew\n\n\n0\n");
    EXPECT_STREQ(memberHead->name, "New");
}

TEST_F(MemberTest, Menu_DeleteOption) {
    mkMember(1, "Old", "1111111", "a@x.com");
    run(memberMenu, "5\n1\n0\n");
    EXPECT_EQ(countMembers(), 0);
}

/* ===================== TESTS FOR transaction.c ===================== */
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
