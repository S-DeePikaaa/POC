/*
 * test_helpers.h - shared fixture and helpers for the Library Management
 *                  System unit tests (GoogleTest).
 *
 * Every test runs inside a fresh, empty scratch directory so the real
 * books.txt / members.txt / transactions.txt are never touched.
 */
#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

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

#endif /* TEST_HELPERS_H */
