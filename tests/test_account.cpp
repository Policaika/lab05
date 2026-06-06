#include <Account.h>
#include <gtest/gtest.h>

class AccountFixture : public testing::Test {
public:
    Account* acc;
    void SetUp() { acc = new Account(123, 1000); }
    void TearDown() { delete acc; }
};

TEST_F(AccountFixture, GetBalance) {
    EXPECT_EQ(acc->GetBalance(), 1000);
}

TEST_F(AccountFixture, ChangeBalanceGood) {
    acc->Lock();
    acc->ChangeBalance(200);
    EXPECT_EQ(acc->GetBalance(), 1200);
}

TEST_F(AccountFixture, ChangeBalanceBad) {
    EXPECT_THROW(acc->ChangeBalance(100), std::runtime_error);
}

TEST_F(AccountFixture, GetID) {
    EXPECT_EQ(acc->id(), 123);
}

TEST_F(AccountFixture, LockTwice) {
    acc->Lock();
    EXPECT_THROW(acc->Lock(), std::runtime_error);
}

TEST_F(AccountFixture, LockAndUnlock) {
    acc->Lock();
    acc->Unlock();
    EXPECT_NO_THROW(acc->Lock());
}
