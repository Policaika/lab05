#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Transaction.h"
#include "classes_with_mock.h"

using ::testing::StrictMock;
using ::testing::Return;
using ::testing::_;
using ::testing::InSequence;

class TransactionTest : public testing::Test {
public:
    StrictMock<MockTransaction>* trans;
    StrictMock<MockAccount>* from;
    StrictMock<MockAccount>* to;

    void SetUp() override {
        trans = new StrictMock<MockTransaction>();
        from = new StrictMock<MockAccount>(1, 1000);
        to = new StrictMock<MockAccount>(2, 1000);
    }

    void TearDown() override {
        delete trans;
        delete from;
        delete to;
    }
};

TEST_F(TransactionTest, DefaultFee) {
    Transaction tx;
    EXPECT_EQ(tx.fee(), 1);
}

TEST_F(TransactionTest, SetFee) {
    Transaction tx;
    tx.set_fee(10);
    EXPECT_EQ(tx.fee(), 10);
}

TEST_F(TransactionTest, SameAccount) {
    EXPECT_THROW(trans->Make(*from, *from, 200), std::logic_error);
}

TEST_F(TransactionTest, NegativeSum) {

    EXPECT_THROW(trans->Make(*from, *to, -100), std::invalid_argument);
}

TEST_F(TransactionTest, TooSmall) {

    EXPECT_THROW(trans->Make(*from, *to, 70), std::logic_error);
}

TEST_F(TransactionTest, FeeExceeds) {
    trans->set_fee(60);

    bool result = trans->Make(*from, *to, 100);
    EXPECT_FALSE(result);
}

TEST_F(TransactionTest, Successful) {
    int sum = 100;
    trans->set_fee(1);

    {
        InSequence seq;

        EXPECT_CALL(*from, Lock());
        EXPECT_CALL(*to, Lock());

        EXPECT_CALL(*to, ChangeBalance(sum));

        EXPECT_CALL(*from, GetBalance()).WillOnce(Return(1000));
        EXPECT_CALL(*from, ChangeBalance(-(sum + 1)));

        EXPECT_CALL(*trans, SaveToDataBase(_, _, sum));

        EXPECT_CALL(*to, Unlock());
        EXPECT_CALL(*from, Unlock());
    }

    bool result = trans->Make(*from, *to, sum);
    EXPECT_TRUE(result);
}

TEST_F(TransactionTest, NotEnoughFunds) {
    int sum = 100;
    trans->set_fee(1);

    {
        InSequence seq;

        EXPECT_CALL(*from, Lock());
        EXPECT_CALL(*to, Lock());

        EXPECT_CALL(*to, ChangeBalance(sum));

        EXPECT_CALL(*from, GetBalance()).WillOnce(Return(50));

        EXPECT_CALL(*to, ChangeBalance(-sum));

        EXPECT_CALL(*trans, SaveToDataBase(_, _, sum));

        EXPECT_CALL(*to, Unlock());
        EXPECT_CALL(*from, Unlock());
    }

    bool result = trans->Make(*from, *to, sum);
    EXPECT_FALSE(result);
}


TEST(TransactionRealTest, RealTransaction) {
    Transaction tx;
    tx.set_fee(1);

    Account from(1, 500);
    Account to(2, 100);

    bool result = tx.Make(from, to, 100);

    EXPECT_TRUE(result);
    EXPECT_EQ(from.GetBalance(), 399);
    EXPECT_EQ(to.GetBalance(), 200);
}
