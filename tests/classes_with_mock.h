#pragma once
#include <gmock/gmock.h>
#include "Account.h"
#include "Transaction.h"

class MockAccount : public Account
{
public:
    MockAccount(int id, int balance) : Account(id, balance) {}
    
    MOCK_METHOD(int, GetBalance, (), (const, override));
    MOCK_METHOD(void, ChangeBalance, (int diff), (override));
    MOCK_METHOD(void, Lock, (), (override));
    MOCK_METHOD(void, Unlock, (), (override));
};

class MockTransaction : public Transaction
{
public:
    MOCK_METHOD(void, SaveToDataBase, (Account& from, Account& to, int sum), (override));
};
