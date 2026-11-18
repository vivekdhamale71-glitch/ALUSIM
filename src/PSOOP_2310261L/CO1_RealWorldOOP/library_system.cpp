// Books, members and a library that holds both and applies the borrowing rules.

#include <string>
#include <vector>

// One book of the library.
class Book {
public:
    Book(int id, const std::string& title) : id_(id), title_(title), borrowedBy_(-1) {
    }

    int id() const {
        return id_;
    }

    const std::string& title() const {
        return title_;
    }

    bool isAvailable() const {
        return borrowedBy_ < 0;
    }

    // the member who has the book, or -1 if it is on the shelf
    int borrowedBy() const {
        return borrowedBy_;
    }

    void lendTo(int memberId) {
        borrowedBy_ = memberId;
    }

    void giveBack() {
        borrowedBy_ = -1;
    }

private:
    int id_;
    std::string title_;
    int borrowedBy_;
};

// One person who may borrow books, at most a few at a time.
class Member {
public:
    Member(int id, const std::string& name) : id_(id), name_(name), borrowed_(0) {
    }

    int id() const {
        return id_;
    }

    const std::string& name() const {
        return name_;
    }

    bool canBorrowMore() const {
        return borrowed_ < LIMIT;
    }

    void borrowedOne() {
        borrowed_ = borrowed_ + 1;
    }

    void returnedOne() {
        borrowed_ = borrowed_ - 1;
    }

private:
    static const int LIMIT = 3;
    int id_;
    std::string name_;
    int borrowed_;
};

// The library HAS books and members (objects inside an object) and applies the rules.
class Library {
public:
    void addBook(const Book& book) {
        books_.push_back(book);
    }

    void addMember(const Member& member) {
        members_.push_back(member);
    }

    // a member borrows a book: both must exist, the book must be on the shelf and the member under the limit
    bool borrow(int memberId, int bookId) {
        Book* book = findBook(bookId);
        Member* member = findMember(memberId);
        if (book == nullptr || member == nullptr) {
            return false;
        }
        if (!book->isAvailable() || !member->canBorrowMore()) {
            return false;
        }
        book->lendTo(memberId);
        member->borrowedOne();
        return true;
    }

    // the book goes back on the shelf
    bool giveBack(int bookId) {
        Book* book = findBook(bookId);
        if (book == nullptr || book->isAvailable()) {
            return false;
        }
        Member* member = findMember(book->borrowedBy());
        if (member != nullptr) {
            member->returnedOne();
        }
        book->giveBack();
        return true;
    }

    // how many books are on the shelf
    int availableCount() const {
        int count = 0;
        for (std::size_t i = 0; i < books_.size(); ++i) {
            if (books_[i].isAvailable()) {
                count = count + 1;
            }
        }
        return count;
    }

private:
    Book* findBook(int id) {
        for (std::size_t i = 0; i < books_.size(); ++i) {
            if (books_[i].id() == id) {
                return &books_[i];
            }
        }
        return nullptr;
    }

    Member* findMember(int id) {
        for (std::size_t i = 0; i < members_.size(); ++i) {
            if (members_[i].id() == id) {
                return &members_[i];
            }
        }
        return nullptr;
    }

    std::vector<Book> books_;
    std::vector<Member> members_;
};
