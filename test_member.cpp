/* test_member.cpp - tests for every function in member.c */
#include "test_helpers.h"

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
