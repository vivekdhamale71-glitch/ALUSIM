// Accounts as classes with private data, inheritance for savings and current accounts, and a bank that holds them.

#include <string>
#include <vector>

// A bank account: the balance is private, so the only way to change it is through
// deposit() and withdraw(), which check the rules (encapsulation).
class Account {
public:
    Account(const std::string& owner, double openingBalance) : owner_(owner), balance_(openingBalance) {
    }

    virtual ~Account() {
    }

    // adds money; a deposit must be positive
    bool deposit(double amount) {
        if (amount <= 0) {
            return false;
        }
        balance_ = balance_ + amount;
        return true;
    }

    // takes money out if the account allows it
    virtual bool withdraw(double amount) {
        if (amount <= 0 || amount > balance_) {
            return false;
        }
        balance_ = balance_ - amount;
        return true;
    }

    double balance() const {
        return balance_;
    }

    const std::string& owner() const {
        return owner_;
    }

protected:
    void setBalance(double value) {
        balance_ = value;
    }

private:
    std::string owner_;
    double balance_;
};

// A savings account is an Account that also earns interest.
class SavingsAccount : public Account {
public:
    SavingsAccount(const std::string& owner, double openingBalance, double ratePercent)
        : Account(owner, openingBalance), ratePercent_(ratePercent) {
    }

    // adds one period of interest to the balance
    void addInterest() {
        double interest = balance() * ratePercent_ / 100.0;
        deposit(interest);
    }

private:
    double ratePercent_;
};

// A current account may go below zero, up to an overdraft limit.
class CurrentAccount : public Account {
public:
    CurrentAccount(const std::string& owner, double openingBalance, double overdraftLimit)
        : Account(owner, openingBalance), overdraftLimit_(overdraftLimit) {
    }

    // same job as Account::withdraw, but with a different rule
    bool withdraw(double amount) override {
        if (amount <= 0 || amount > balance() + overdraftLimit_) {
            return false;
        }
        setBalance(balance() - amount);
        return true;
    }

private:
    double overdraftLimit_;
};

// A bank owns many accounts (composition) and moves money between them.
class Bank {
public:
    ~Bank() {
        for (std::size_t i = 0; i < accounts_.size(); ++i) {
            delete accounts_[i];
        }
    }

    // the bank takes ownership of the account
    void open(Account* account) {
        accounts_.push_back(account);
    }

    // finds an account by its owner's name, or nullptr
    Account* find(const std::string& owner) {
        for (std::size_t i = 0; i < accounts_.size(); ++i) {
            if (accounts_[i]->owner() == owner) {
                return accounts_[i];
            }
        }
        return nullptr;
    }

    // moves money from one owner to another; nothing changes if it cannot be done
    bool transfer(const std::string& from, const std::string& to, double amount) {
        Account* source = find(from);
        Account* target = find(to);
        if (source == nullptr || target == nullptr) {
            return false;
        }
        if (!source->withdraw(amount)) {
            return false;
        }
        target->deposit(amount);
        return true;
    }

    // the sum of all balances
    double totalMoney() const {
        double total = 0;
        for (std::size_t i = 0; i < accounts_.size(); ++i) {
            total = total + accounts_[i]->balance();
        }
        return total;
    }

private:
    std::vector<Account*> accounts_;
};
