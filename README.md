# Лабораторная работа 5

Для начала пройдемся по каждому файлу прокта:

# 1 .github/workflows/ci.yml

```yaml
name: for coverage

on: push

jobs:
  coverage:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      - name: install dependencies
        run: sudo apt-get update && sudo apt-get install -y lcov gcc g++
      - name: configure cmake
        env:
          CXX: g++
        run: cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DCODE_COVERAGE=ON
      - name: build
        run: cmake --build build
      - name: tests
        run: ctest --test-dir build --output-on-failure
      - name: coverage
        run: |
          lcov --capture --directory build --output-file coverage.info --ignore-errors mismatch
          echo "=== Files in coverage.info ==="
          lcov --list coverage.info
          lcov --extract coverage.info '*/lab05/banking/*' '*/banking/*' --output-file coverage_result.info --ignore-errors mismatch,unused
          lcov --list coverage_result.info
      - name: push to coveralls
        uses: coverallsapp/github-action@v2
        with:
          github-token: ${{secrets.GITHUB_TOKEN}}
          file: coverage_result.info
```
В данном случае тесты запускаются на только на ubuntu, сначала мы устанавливаем сборку linux:

```yaml
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
```

Дальше мы устанавливаем lcov, gcc, g++ и собираем файлики в папку build, то есть создаются статические библиотеки, также включаем флаги -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DCODE_COVERAGE=ON для тестов, билдим проект, то есть создаем исполняемый файл, при помощи ультилитки ctest мы делаем тесты и при помощи lcov мы взаимодействуем c тестами 

1. Сбор сырых данных
--capture — захватывает данные о покрытии
--directory build — смотрит в папке build (там находятся .gcda файлы с данными о выполнении)
--output-file coverage_raw.info — сохраняет в файл coverage_raw.info
--ignore-errors mismatch — игнорирует ошибки несоответствия версий gcov

2. Извлечение только нужных файлов
--extract — оставляет только указанные файлы
'*/banking/Account.cpp' '*/banking/Transaction.cpp' — паттерны для ваших исходных файлов
--output-file coverage.info — перезаписывает файл

3. Удаление системных файлов
--remove — удаляет файлы по паттерну
'/usr/*' — все системные заголовки из /usr/include/
--ignore-errors unused — игнорирует ошибку, если паттерн не найден


## 2 CMakeLists.txt

Переходим к корневому CMakeLists:

```cmake
cmake_minimum_required(VERSION 3.10)

project(lab05)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(banking)

option(BUILD_TESTS "Build unit tests" ON)
option(CODE_COVERAGE "Coverage report" ON)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(third-party/gtest)
  file(GLOB ${PROJECT_NAME}_TEST tests/*.cpp)
  add_executable(banking_test ${${PROJECT_NAME}_TEST})
  target_include_directories(banking_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/banking ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_link_libraries(banking_test ${PROJECT} banking gtest gtest_main gmock)
    if(CODE_COVERAGE)
      target_compile_options(banking_test PRIVATE --coverage -O0 -g)
      target_link_options(banking_test PRIVATE --coverage)
    endif()
  add_test(NAME banking_test COMMAND banking_test)
endif()
```

option(BUILD_TESTS "Build unit tests" ON)
option(CODE_COVERAGE "Coverage report" ON)
Это две опции, которые фактически создают две переменных, в которых хранится OFF/ON, в нашем случае мы включаем тесты(ON).

Дальше идет мы проверяем, если BUILD_TESTS=ON, то исполняется код ниже до endif(). 

1) enable_testing - активирует систему тестирования CMake
2) add_subdirectory(third-party/gtest) - добавляет папку third-party/gtest в сборку, он нужен для gmock, gtest
3) file(GLOB ${PROJECT_NAME}_TEST tests/*.cpp) - самое интересное, подобное задание переменной GLOB немного устарело, однако в лабораторных работах использование вполне обоснованно и даже более понятно на мой взгляд, то есть все файлы с расширение .cpp закидываются путями в GLOB.
4) Дальше все по стандарту, делаем запускаемый файл, подрубаем библиотеки, указываем где они находятся и линкуем библиотеки
5) Включение покрытия, если мы при cmake -S . -B включаем переменную CODE_COVERAGE=ON, то выполняется блок кода, где 

а) --coverage — вставляет счётчики выполнения, генерирует .gcno файлы
б) -O0 — отключает оптимизации (иначе покрытие не будет достоверным)
в) -g — добавляет отладочную информацию

add_test() — регистрирует тест в CTest
NAME banking_test — имя теста (видно в выводе ctest)
COMMAND banking_test — команда для запуска

## 3 banking/CMakeLists.txt

CMake в banking создает статическую библиотеку banking из двух файлов. Если включено покрытие — добавляет те же флаги --coverage.

```cmake
add_library(banking STATIC Account.cpp Transaction.cpp)
target_include_directories(banking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CODE_COVERAGE)
    target_compile_options(banking PRIVATE -O0 -g --coverage)
    target_link_options(banking PRIVATE --coverage)
endif()
```


## 4 banking/Account.h и Account.cpp

Account.h:

```cxx
#pragma once

class Account {
 public:
  Account(int id, int balance);
  virtual ~Account();

  virtual int GetBalance() const;
  virtual void ChangeBalance(int diff);
  virtual void Lock();
  virtual void Unlock();

  int id() const { return id_; }

 private:
  int id_;
  int balance_;
  bool is_locked_;
};
```
Все методы, которые будут мокаться в тестах, объявлены virtual
Виртуальный деструктор — обязателен для корректного удаления наследников
Метод id() не виртуальный (inline в заголовке)


Account.cpp:

```cxx
#include "Account.h"

#include <stdexcept>

Account::Account(int id, int balance)
    : id_(id), balance_(balance), is_locked_(false) {}

Account::~Account() {}

int Account::GetBalance() const { return balance_; }

void Account::ChangeBalance(int diff) {
  if (!is_locked_) throw std::runtime_error("at first lock the account");
  balance_ += diff;
}

void Account::Lock() {
  if (is_locked_) throw std::runtime_error("already locked");
  is_locked_ = true;
}

void Account::Unlock() { is_locked_ = false; }
```

## 5 banking/Transaction.h и Transaction.cpp

Transaction.h:

```cxx
#pragma once
class Account;

class Transaction {
 public:
  Transaction();
  virtual ~Transaction();

  bool Make(Account& from, Account& to, int sum);

  int fee() const { return fee_; }
  void set_fee(int fee) { fee_ = fee; }

 private:
  void Credit(Account& accout, int sum);
  bool Debit(Account& accout, int sum);

  virtual void SaveToDataBase(Account& from, Account& to, int sum);

  int fee_;
};
```

Transaction.cpp:

```cxx
namespace {
struct Guard {
  Guard(Account& account) : account_(&account) { account_->Lock(); }
  ~Guard() { account_->Unlock(); }
 private:
  Account* account_;
};
}

bool Transaction::Make(Account& from, Account& to, int sum) {
  if (from.id() == to.id()) throw std::logic_error("invalid action");
  if (sum < 0) throw std::invalid_argument("sum can't be negative");
  if (sum < 100) throw std::logic_error("too small");
  if (fee_ * 2 > sum) return false;

  Guard guard_from(from);
  Guard guard_to(to);

  Credit(to, sum);
  bool success = Debit(from, sum + fee_);
  if (!success) to.ChangeBalance(-sum);

  SaveToDataBase(from, to, sum);
  return success;
}
```

RAII-класс Guard — автоматически блокирует счёт при создании и разблокирует при уничтожении (даже при исключениях)
Проверки: нельзя переводить самому себе, сумма должна быть ≥100, комиссия не должна превышать половину суммы
Если списание не удалось — откатываем зачисление получателю.


## 6 tests/mock_classes.h

```cxx
#pragma once
#include <gmock/gmock.h>
#include "Account.h"
#include "Transaction.h"

class MockAccount : public Account {
public:
    MockAccount(int id, int balance) : Account(id, balance) {}
    MOCK_METHOD(int, GetBalance, (), (const, override));
    MOCK_METHOD(void, ChangeBalance, (int diff), (override));
    MOCK_METHOD(void, Lock, (), (override));
    MOCK_METHOD(void, Unlock, (), (override));
};

class MockTransaction : public Transaction {
public:
    MOCK_METHOD(void, SaveToDataBase, (Account& from, Account& to, int sum), (override));
};
```

Mock-классы для подмены реальных объектов в тестах:

    MOCK_METHOD — макрос Google Mock, создающий заглушку
    Позволяют проверять, какие методы и сколько раз вызывались
    
    
## 7 tests/test_account.cpp

```cxx
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
```
class AccountFixture : public testing::Test:

```
Наследуется от testing::Test — базовый класс фикстур Google Test
acc — указатель на объект Account, общий для всех тестов в этой фикстуре
SetUp() — вызывается перед каждым тестом. Создаёт новый счёт с id=123 и balance=1000. TearDown() — вызывается после каждого теста. Удаляет объект, чтобы не было утечек памяти. SetUp() и TearDown() выполняются для каждого теста отдельно, то есть каждый новый объект получает параметры на вход Account(123, 1000)
```
Дальше идут сами тесты:

Тест 1: Проверка начального баланса

1) TEST_F — тест с фикстурой (первый аргумент — имя фикстуры, второй — имя теста).
2) EXPECT_EQ — проверяет равенство. Если не равно — тест не падает, а только отмечает ошибку.

Тест 2: Успешное изменение баланса

1) Сначала блокируем счет, потом меняем баланс и проверяем результат


Тест 3: Попытка изменить без блокировки

1) EXPECT_THROW(выражение, тип_исключения) — проверяет, что выражение выбрасывает исключение указанного типа, если счёт не заблокирован, то ChangeBalance должен выбросить std::runtime_error, если же исключение не выброшено или выброшено другое, то тест провалится.

Тест 4: Проверка ID

1) Проверяем, что метод id() возвращает 123 (то, что передали в конструктор)

Тест 5: Двойная блокировка

1) Локаем первый раз - успешно
2) Второй раз локаем - должен выдать исключение(нельзя заблокировать счет дважды)

Тест 6: Блокировка → Разблокировка → Блокировка

1) EXPECT_NO_THROW(выражение) — проверяет, что выражение НЕ выбрасывает исключение


## 8 tests/test_transaction.cpp

```cxx
#include <Account.h>
#include <Transaction.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class AccountMock : public Account {
public:
    AccountMock(int id, int balance) : Account(id, balance) {}

    MOCK_METHOD(int, GetBalance, (), (const, override));
    MOCK_METHOD(void, ChangeBalance, (int diff), (override));
    MOCK_METHOD(void, Lock, (), (override));
    MOCK_METHOD(void, Unlock, (), (override));
};

class TransactionFixture : public testing::Test {
public:
    Transaction* tr;
    AccountMock* from;
    AccountMock* to;
    void SetUp () override {
        tr = new Transaction;
        from = new testing::NiceMock<AccountMock>(1, 1000);
        to = new testing::NiceMock<AccountMock>(2, 1000);
    }
    void TearDown () override {
        delete tr;
        delete from;
        delete to;
    }
};

TEST_F(TransactionFixture, Fee) {
    EXPECT_EQ(tr->fee(), 1);
}

TEST_F(TransactionFixture, SetFee) {
    tr->set_fee(10);
    EXPECT_EQ(tr->fee(), 10);
}

TEST_F(TransactionFixture, SuccessfulTransfer) {
    EXPECT_CALL(*to, ChangeBalance(200)).Times(1);
    EXPECT_CALL(*from, GetBalance()).WillOnce(testing::Return(1000)).WillRepeatedly(testing::Return(799));
    EXPECT_CALL(*to, GetBalance()).WillRepeatedly(testing::Return(1200));

    EXPECT_TRUE(tr->Make(*from, *to, 200));
}

TEST_F(TransactionFixture, TransferToYourself) {
    EXPECT_THROW(tr->Make(*from, *from, 200), std::logic_error);
}

TEST_F(TransactionFixture, NegativeSumTransfer) {
    EXPECT_THROW(tr->Make(*from, *to, -100), std::invalid_argument);
}

TEST_F(TransactionFixture, TooSmallSumTransfer) {
    EXPECT_THROW(tr->Make(*from, *to, 70), std::logic_error);
}

TEST_F(TransactionFixture, TooBigFee) {
    tr->set_fee(60);
    EXPECT_FALSE(tr->Make(*from, *to, 100));
}

TEST_F(TransactionFixture, TooBigSumTransfer) {
    EXPECT_CALL(*from, GetBalance()).WillRepeatedly(testing::Return(1000));
    EXPECT_CALL(*to, ChangeBalance(1200)).Times(1);
    EXPECT_CALL(*to, ChangeBalance(-1200)).Times(1);
    EXPECT_CALL(*to, GetBalance()).WillRepeatedly(testing::Return(1000));
    EXPECT_FALSE(tr->Make(*from, *to, 1200));
}
```

Поговорим подробнее о class TransactionFixture : public testing::Test,
тут я использовал NiceMock, т.к главным его плюсом является подавление предупреждения о неинтересных вызовах, то есть позволяет не засорять логи сборки, по умолчанию в gMock, если тестируемый код вызывает метод мок-объекта, который не представляет интереса для конкретного теста, фреймворк выдает предупреждение, которые и засоряют логи. NiceMock решает данную проблему игнорируя любые вызовы методов.

Вообще Google Mock имеет три типа моков:
NiceMock<T> - Молча игнорирует(Выдает ошибку, но тест проходит)
Mock<T> - Предупреждает(Выдает ошибку)
StrictMock<T> - Выкидывает исключение(Тест падет)

Тест 1: Комиссия по умолчанию:

1) Создаётся Transaction в SetUp().
2) Конструктор Transaction устанавливает fee_ = 1 (по умолчанию). 
3) Проверяем, что fee() возвращает 1

Тест 2: Установка комиссии.
Тест 3: Успешный перевод:

EXPECT_CALL(*to, ChangeBalance(200)).Times(1);

Ожидаем, что у объекта to будет вызван ChangeBalance(200) ровно 1 раз

EXPECT_CALL(*from, GetBalance()).WillOnce(Return(1000)).WillRepeatedly(Return(799));

Первый вызов GetBalance() вернёт 1000 (начальный баланс отправителя)
Все последующие вызовы вернут 799 (1000 - 200 - 1 комиссия = 799)
    
Первый вызов GetBalance() проверяет, хватает ли средств (1000 > 201? да). После списания баланс становится 799.

EXPECT_CALL(*to, GetBalance()).WillRepeatedly(Return(1200));

Все вызовы GetBalance() у получателя вернут 1200 (1000 + 200)

EXPECT_TRUE(tr->Make(*from, *to, 200));

Вызываем перевод 200 от from к to
Ожидаем, что вернёт true

    
Тест 4: Перевод самому себе:

1) Передаем объект как отправителя и как получателя, итогом ожидаем исключение std::logic_error

Тест 5: Отрицательная сумма

Тест 6: Слишком маленькая сумма:
То есть сумма меньше минимума.

Тест 7: Комиссия слишком большая:

Устанавливаем комиссию 60
Сумма перевода 100
Комиссия больше половины суммы, значит транзакция невозможна
Вывод: False

Тест 8: Недостаточно средств (TooBigSumTransfer):

У отправителя баланс 1000
Пытаемся перевести 1200 (больше, чем есть)
Комиссия по умолчанию = 1, значит нужно списать 1201
Возвращает False


## Результаты сборки и тестирования

```zsh
┌──(p㉿Policai)-[~/…/Policaika/workspace/reports/lab05]
└─$ cmake -S . -B build -DBUILD_TESTS=ON -DCODE_COVERAGE=ON
-- The C compiler identification is GNU 15.2.0
-- The CXX compiler identification is GNU 15.2.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE
-- Configuring done (0.5s)
-- Generating done (0.0s)
-- Build files have been written to: /home/p/Рабочий стол/Policaika/workspace/reports/lab05/build
```                       

```zsh
┌──(p㉿Policai)-[~/…/Policaika/workspace/reports/lab05]
└─$ cmake --build build                                    
[  7%] Building CXX object third-party/gtest/googletest/CMakeFiles/gtest.dir/src/gtest-all.cc.o
[ 14%] Linking CXX static library ../../../lib/libgtest.a
[ 14%] Built target gtest
[ 21%] Building CXX object third-party/gtest/googletest/CMakeFiles/gtest_main.dir/src/gtest_main.cc.o
[ 28%] Linking CXX static library ../../../lib/libgtest_main.a
[ 28%] Built target gtest_main
[ 35%] Building CXX object banking/CMakeFiles/banking.dir/Account.cpp.o
[ 42%] Building CXX object banking/CMakeFiles/banking.dir/Transaction.cpp.o
[ 50%] Linking CXX static library libbanking.a
[ 50%] Built target banking
[ 57%] Building CXX object third-party/gtest/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o
[ 64%] Linking CXX static library ../../../lib/libgmock.a
[ 64%] Built target gmock
[ 71%] Building CXX object CMakeFiles/banking_test.dir/tests/test_account.cpp.o
[ 78%] Building CXX object CMakeFiles/banking_test.dir/tests/test_transaction.cpp.o
[ 85%] Linking CXX executable banking_test
[ 85%] Built target banking_test
[ 92%] Building CXX object third-party/gtest/googlemock/CMakeFiles/gmock_main.dir/src/gmock_main.cc.o
[100%] Linking CXX static library ../../../lib/libgmock_main.a
[100%] Built target gmock_main
```

```bash
┌──(p㉿Policai)-[~/…/Policaika/workspace/reports/lab05]
└─$ ctest --test-dir build --output-on-failure --verbose                                 
UpdateCTestConfiguration  from :/home/p/Рабочий стол/Policaika/workspace/reports/lab05/build/DartConfiguration.tcl
Test project /home/p/Рабочий стол/Policaika/workspace/reports/lab05/build
Constructing a list of tests
Done constructing a list of tests
Updating test list for fixtures
Added 0 tests to meet fixture requirements
Checking test dependency graph...
Checking test dependency graph end
test 1
    Start 1: banking_test

1: Test command: /home/p/Рабочий\ стол/Policaika/workspace/reports/lab05/build/banking_test
1: Working Directory: /home/p/Рабочий стол/Policaika/workspace/reports/lab05/build
1: Test timeout computed to be: 10000000
1: Running main() from /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googletest/src/gtest_main.cc
1: [==========] Running 14 tests from 2 test suites.
1: [----------] Global test environment set-up.
1: [----------] 6 tests from AccountFixture
1: [ RUN      ] AccountFixture.GetBalance
1: [       OK ] AccountFixture.GetBalance (0 ms)
1: [ RUN      ] AccountFixture.ChangeBalanceGood
1: [       OK ] AccountFixture.ChangeBalanceGood (0 ms)
1: [ RUN      ] AccountFixture.ChangeBalanceBad
1: [       OK ] AccountFixture.ChangeBalanceBad (0 ms)
1: [ RUN      ] AccountFixture.GetID
1: [       OK ] AccountFixture.GetID (0 ms)
1: [ RUN      ] AccountFixture.LockTwice
1: [       OK ] AccountFixture.LockTwice (0 ms)
1: [ RUN      ] AccountFixture.LockAndUnlock
1: [       OK ] AccountFixture.LockAndUnlock (0 ms)
1: [----------] 6 tests from AccountFixture (0 ms total)
1: 
1: [----------] 8 tests from TransactionFixture
1: [ RUN      ] TransactionFixture.Fee
1: [       OK ] TransactionFixture.Fee (0 ms)
1: [ RUN      ] TransactionFixture.SetFee
1: [       OK ] TransactionFixture.SetFee (0 ms)
1: [ RUN      ] TransactionFixture.SuccessfulTransfer
1: 1 send to 2 $200
1: Balance 1 is 799
1: Balance 2 is 1200
1: [       OK ] TransactionFixture.SuccessfulTransfer (0 ms)
1: [ RUN      ] TransactionFixture.TransferToYourself
1: [       OK ] TransactionFixture.TransferToYourself (0 ms)
1: [ RUN      ] TransactionFixture.NegativeSumTransfer
1: [       OK ] TransactionFixture.NegativeSumTransfer (0 ms)
1: [ RUN      ] TransactionFixture.TooSmallSumTransfer
1: [       OK ] TransactionFixture.TooSmallSumTransfer (0 ms)
1: [ RUN      ] TransactionFixture.TooBigFee
1: [       OK ] TransactionFixture.TooBigFee (0 ms)
1: [ RUN      ] TransactionFixture.TooBigSumTransfer
1: 1 send to 2 $1200
1: Balance 1 is 1000
1: Balance 2 is 1000
1: [       OK ] TransactionFixture.TooBigSumTransfer (0 ms)
1: [----------] 8 tests from TransactionFixture (0 ms total)
1: 
1: [----------] Global test environment tear-down
1: [==========] 14 tests from 2 test suites ran. (0 ms total)
1: [  PASSED  ] 14 tests.
1/1 Test #1: banking_test .....................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.01 sec
```

Результаты тестов по всей библиотеки:

```zsh
┌──(p㉿Policai)-[~/…/Policaika/workspace/reports/lab05]
└─$ lcov --capture --directory build --output-file coverage_raw.info --ignore-errors mismatch,inconsistent
Capturing coverage data from build
geninfo cmd: '/usr/bin/geninfo build --toolname lcov --output-filename coverage_raw.info --ignore-errors mismatch --ignore-errors inconsistent'
Found gcov version: 15.2.0
Using intermediate gcov format
Recording 'internal' directories:
        /home/p/Рабочий стол/Policaika/workspace/reports/lab05/build
        build
Writing temporary data to /tmp/geninfo_datf6zy
Scanning build for .gcda files ...
Found 4 data files in build
using: chunkSize: 1, nchunks:4, intervalLength:0
lcov: WARNING: (inconsistent) mismatched end line for _ZN30AccountFixture_GetBalance_Test8TestBodyEv at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:11: 11 -> 13 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN30AccountFixture_GetBalance_TestD0Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:11: 13 -> 11 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN30AccountFixture_GetBalance_TestD2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:11: 13 -> 11 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN37AccountFixture_ChangeBalanceGood_Test8TestBodyEv at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:15: 15 -> 19 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN37AccountFixture_ChangeBalanceGood_TestD2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:15: 19 -> 15 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN37AccountFixture_ChangeBalanceGood_TestD0Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:15: 19 -> 15 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN36AccountFixture_ChangeBalanceBad_Test8TestBodyEv at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:21: 21 -> 23 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN36AccountFixture_ChangeBalanceBad_TestD2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:21: 23 -> 21 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN25AccountFixture_GetID_Test8TestBodyEv at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:25: 25 -> 27 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN25AccountFixture_GetID_TestD0Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:25: 27 -> 25 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN25AccountFixture_GetID_TestC2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:25: 27 -> 25 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN29AccountFixture_LockTwice_Test8TestBodyEv at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:29: 29 -> 32 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN29AccountFixture_LockTwice_TestC2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:29: 32 -> 29 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN33AccountFixture_LockAndUnlock_Test8TestBodyEv at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:34: 34 -> 38 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN33AccountFixture_LockAndUnlock_TestC2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp:34: 38 -> 34 while capturing from build/CMakeFiles/banking_test.dir/tests/test_account.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN27TransactionFixture_Fee_Test8TestBodyEv at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:33: 33 -> 35 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN30TransactionFixture_SetFee_Test8TestBodyEv at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:37: 37 -> 40 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN30TransactionFixture_SetFee_TestD0Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:37: 40 -> 37 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN30TransactionFixture_SetFee_TestD2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:37: 40 -> 37 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN42TransactionFixture_SuccessfulTransfer_Test8TestBodyEv at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:42: 42 -> 48 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN42TransactionFixture_TransferToYourself_TestC2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:50: 52 -> 50 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN42TransactionFixture_TransferToYourself_TestD0Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:50: 52 -> 50 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN42TransactionFixture_TransferToYourself_TestD2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:50: 52 -> 50 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN43TransactionFixture_NegativeSumTransfer_TestC2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:54: 56 -> 54 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN43TransactionFixture_NegativeSumTransfer_TestD2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:54: 56 -> 54 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN43TransactionFixture_NegativeSumTransfer_TestD0Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:54: 56 -> 54 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN43TransactionFixture_TooSmallSumTransfer_TestD2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:58: 60 -> 58 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN43TransactionFixture_TooSmallSumTransfer_TestD0Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:58: 60 -> 58 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN43TransactionFixture_TooSmallSumTransfer_TestC2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:58: 60 -> 58 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN33TransactionFixture_TooBigFee_TestD2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:62: 65 -> 62 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN33TransactionFixture_TooBigFee_TestD0Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:62: 65 -> 62 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN33TransactionFixture_TooBigFee_TestC2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:62: 65 -> 62 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN41TransactionFixture_TooBigSumTransfer_TestD2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:67: 73 -> 67 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN41TransactionFixture_TooBigSumTransfer_TestD0Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:67: 73 -> 67 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
lcov: WARNING: (inconsistent) mismatched end line for _ZN41TransactionFixture_TooBigSumTransfer_TestC2Ev at /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp:67: 73 -> 67 while capturing from build/CMakeFiles/banking_test.dir/tests/test_transaction.cpp.gcda
Finished processing 4 GCDA files
Apply filtering..
Finished filter file processing
Finished .info-file creation
Summary coverage rate:
  source files: 54
  lines.......: 69.4% (1338 of 1929 lines)
  functions...: 73.2% (689 of 941 functions)
Message summary:
  35 warning messages:
    inconsistent: 35
  1 ignore message:
    inconsistent: 1
```               
               

Посмотрим результаты тестов по нашим файлам:

```zsh
┌──(p㉿Policai)-[~/…/Policaika/workspace/reports/lab05]
└─$ lcov --extract coverage_raw.info '*/banking/Account.cpp' '*/banking/Transaction.cpp' --output-file coverage.info
lcov --list coverage.info

Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/banking/Account.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/banking/Transaction.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_account.cpp
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/tests/test_transaction.cpp
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googlemock/include/gmock/gmock-actions.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googlemock/include/gmock/gmock-cardinalities.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googlemock/include/gmock/gmock-matchers.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googlemock/include/gmock/gmock-nice-strict.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googlemock/include/gmock/gmock-spec-builders.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googlemock/include/gmock/internal/gmock-internal-utils.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googletest/include/gtest/gtest-assertion-result.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googletest/include/gtest/gtest-matchers.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googletest/include/gtest/gtest-message.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googletest/include/gtest/gtest-printers.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googletest/include/gtest/gtest.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googletest/include/gtest/internal/gtest-internal.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googletest/include/gtest/internal/gtest-port.h
Excluding /home/p/Рабочий стол/Policaika/workspace/reports/lab05/third-party/gtest/googletest/include/gtest/internal/gtest-type-util.h
Excluding /usr/include/c++/15/bits/alloc_traits.h
Excluding /usr/include/c++/15/bits/allocated_ptr.h
Excluding /usr/include/c++/15/bits/allocator.h
Excluding /usr/include/c++/15/bits/atomic_base.h
Excluding /usr/include/c++/15/bits/basic_string.h
Excluding /usr/include/c++/15/bits/basic_string.tcc
Excluding /usr/include/c++/15/bits/char_traits.h
Excluding /usr/include/c++/15/bits/invoke.h
Excluding /usr/include/c++/15/bits/move.h
Excluding /usr/include/c++/15/bits/new_allocator.h
Excluding /usr/include/c++/15/bits/ptr_traits.h
Excluding /usr/include/c++/15/bits/shared_ptr.h
Excluding /usr/include/c++/15/bits/shared_ptr_base.h
Excluding /usr/include/c++/15/bits/std_function.h
Excluding /usr/include/c++/15/bits/stl_algobase.h
Excluding /usr/include/c++/15/bits/stl_construct.h
Excluding /usr/include/c++/15/bits/stl_function.h
Excluding /usr/include/c++/15/bits/stl_iterator.h
Excluding /usr/include/c++/15/bits/stl_iterator_base_funcs.h
Excluding /usr/include/c++/15/bits/stl_iterator_base_types.h
Excluding /usr/include/c++/15/bits/stl_set.h
Excluding /usr/include/c++/15/bits/stl_tree.h
Excluding /usr/include/c++/15/bits/stl_uninitialized.h
Excluding /usr/include/c++/15/bits/stl_vector.h
Excluding /usr/include/c++/15/bits/unique_ptr.h
Excluding /usr/include/c++/15/bits/vector.tcc
Excluding /usr/include/c++/15/ext/aligned_buffer.h
Excluding /usr/include/c++/15/ext/alloc_traits.h
Excluding /usr/include/c++/15/ext/atomicity.h
Excluding /usr/include/c++/15/new
Excluding /usr/include/c++/15/string_view
Excluding /usr/include/c++/15/tuple
Excluding /usr/include/c++/15/typeinfo
Excluding /usr/include/x86_64-linux-gnu/c++/15/bits/c++config.h
Removed 52 files
Writing data to coverage.info
Summary coverage rate:
  source files: 2
  lines.......: 100.0% (46 of 46 lines)
  functions...: 100.0% (16 of 16 functions)
Message summary:
  no messages were reported
                      |Lines       |Functions  
Filename              |Rate     Num|Rate    Num
===============================================
[/home/p/Рабочий стол/Policaika/workspace/reports/lab05/banking/]
Account.cpp           | 100%     13| 100%     7
Transaction.cpp       | 100%     33| 100%     9
===============================================
                Total:| 100%     46| 100%    16
Message summary:
  no messages were reported
```
