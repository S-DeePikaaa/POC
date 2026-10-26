/* test_utils.cpp - tests for every function in utils.c */
#include "test_helpers.h"

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
