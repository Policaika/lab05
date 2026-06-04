#include <gtest/gtest.h>
#include "Account.h"

class AccountBalanceTest : public testing::TestWithParam<int> {
public:
    Account* acc;

    void SetUp() override {
        acc = new Account(1, GetParam());
    }


    void TearDown() override {
        delete acc;
    }
};

TEST_P(AccountBalanceTest, InitialBalance) {
    EXPECT_EQ(acc->GetBalance(), GetParam());
}

INSTANTIATE_TEST_SUITE_P(
    DifferentBalances,
    AccountBalanceTest,
    testing::Values(100, 0, 1000, 5000)
    );

class AccountBehaviourTest : public testing::Test {
public:
    Account* acc;

    void SetUp() override {
        acc = new Account(123, 1000);
    }

    void TearDown() override {
        delete acc;
    }
};

TEST_F(AccountBehaviourTest, LockUnlock) {
    EXPECT_NO_THROW(acc->Lock());
    EXPECT_THROW(acc->Lock(), std::runtime_error);
    acc->Unlock();
    EXPECT_NO_THROW(acc->Lock());
}

TEST_F(AccountBehaviourTest, ChangeBalanceWhenUnlocked) {
    EXPECT_THROW(acc->ChangeBalance(50), std::runtime_error);
}

TEST_F(AccountBehaviourTest, ChangeBalanceWhenLocked) {
    acc->Lock();
    EXPECT_NO_THROW(acc->ChangeBalance(200));
    EXPECT_EQ(acc->GetBalance(), 1200);
}

TEST_F(AccountBehaviourTest, IdIsCorrect) {
    EXPECT_EQ(acc->id(), 123);
}
