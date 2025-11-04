#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>

using namespace std;

// Account structure
struct Account {
    char userID[31];
    char password[31];
    char username[31];
    int privilege;
    
    Account() : privilege(0) {
        memset(userID, 0, sizeof(userID));
        memset(password, 0, sizeof(password));
        memset(username, 0, sizeof(username));
    }
};

// Book structure
struct Book {
    char ISBN[21];
    char bookName[61];
    char author[61];
    char keyword[61];
    double price;
    int quantity;
    
    Book() : price(0.0), quantity(0) {
        memset(ISBN, 0, sizeof(ISBN));
        memset(bookName, 0, sizeof(bookName));
        memset(author, 0, sizeof(author));
        memset(keyword, 0, sizeof(keyword));
    }
    
    bool operator<(const Book& other) const {
        return strcmp(ISBN, other.ISBN) < 0;
    }
};

// Transaction record
struct Transaction {
    double amount;
    bool isIncome; // true for income (buy), false for expenditure (import)
};

// Global data structures
map<string, Account> accounts;
map<string, Book> books;
vector<Transaction> transactions;
vector<pair<string, string>> operationLog; // (userID, operation)

// Login stack
struct LoginSession {
    string userID;
    string selectedISBN;
};
vector<LoginSession> loginStack;

// File paths
const string ACCOUNT_FILE = "accounts.dat";
const string BOOK_FILE = "books.dat";
const string TRANSACTION_FILE = "transactions.dat";
const string LOG_FILE = "log.dat";

// Helper functions
void trim(string& s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
}

vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool isValidUserID(const string& s) {
    if (s.empty() || s.length() > 30) return false;
    for (char c : s) {
        if (!isalnum(c) && c != '_') return false;
    }
    return true;
}

bool isValidPassword(const string& s) {
    return isValidUserID(s);
}

bool isValidUsername(const string& s) {
    if (s.empty() || s.length() > 30) return false;
    for (char c : s) {
        if (c < 32 || c > 126) return false;
    }
    return true;
}

bool isValidISBN(const string& s) {
    if (s.empty() || s.length() > 20) return false;
    for (char c : s) {
        if (c < 33 || c > 126) return false;
    }
    return true;
}

bool isValidBookName(const string& s) {
    if (s.empty() || s.length() > 60) return false;
    for (char c : s) {
        // ASCII characters except invisible characters (0-31, 127) and double quote (34)
        if (c < 32 || c > 126 || c == '"') return false;
    }
    return true;
}

bool isValidKeyword(const string& s) {
    if (s.empty() || s.length() > 60) return false;
    for (char c : s) {
        // ASCII characters except invisible characters (0-31, 127) and double quote (34)
        if (c < 32 || c > 126 || c == '"') return false;
    }
    // Check for duplicate keywords
    vector<string> keywords = split(s, '|');
    set<string> uniqueKeywords(keywords.begin(), keywords.end());
    if (keywords.size() != uniqueKeywords.size()) return false;
    // Check each keyword is non-empty
    for (const auto& kw : keywords) {
        if (kw.empty()) return false;
    }
    return true;
}

bool isValidPrice(const string& s) {
    if (s.empty() || s.length() > 13) return false;
    int dotCount = 0;
    for (char c : s) {
        if (c == '.') {
            dotCount++;
            if (dotCount > 1) return false;
        } else if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

bool isValidQuantity(const string& s) {
    if (s.empty() || s.length() > 10) return false;
    for (char c : s) {
        if (!isdigit(c)) return false;
    }
    long long val = stoll(s);
    return val > 0 && val <= 2147483647;
}

int getCurrentPrivilege() {
    if (loginStack.empty()) return 0;
    return accounts[loginStack.back().userID].privilege;
}

string getCurrentUserID() {
    if (loginStack.empty()) return "";
    return loginStack.back().userID;
}

void loadData() {
    // Load accounts
    ifstream accountFile(ACCOUNT_FILE, ios::binary);
    if (accountFile.is_open()) {
        Account acc;
        while (accountFile.read((char*)&acc, sizeof(Account))) {
            accounts[acc.userID] = acc;
        }
        accountFile.close();
    }
    
    // Load books
    ifstream bookFile(BOOK_FILE, ios::binary);
    if (bookFile.is_open()) {
        Book book;
        while (bookFile.read((char*)&book, sizeof(Book))) {
            books[book.ISBN] = book;
        }
        bookFile.close();
    }
    
    // Load transactions
    ifstream transFile(TRANSACTION_FILE, ios::binary);
    if (transFile.is_open()) {
        Transaction trans;
        while (transFile.read((char*)&trans, sizeof(Transaction))) {
            transactions.push_back(trans);
        }
        transFile.close();
    }
}

void saveAccounts() {
    ofstream accountFile(ACCOUNT_FILE, ios::binary | ios::trunc);
    for (const auto& pair : accounts) {
        accountFile.write((const char*)&pair.second, sizeof(Account));
    }
    accountFile.close();
}

void saveBooks() {
    ofstream bookFile(BOOK_FILE, ios::binary | ios::trunc);
    for (const auto& pair : books) {
        bookFile.write((const char*)&pair.second, sizeof(Book));
    }
    bookFile.close();
}

void saveTransactions() {
    ofstream transFile(TRANSACTION_FILE, ios::binary | ios::trunc);
    for (const auto& trans : transactions) {
        transFile.write((const char*)&trans, sizeof(Transaction));
    }
    transFile.close();
}

void initialize() {
    loadData();
    
    // Create root account if it doesn't exist
    if (accounts.find("root") == accounts.end()) {
        Account root;
        strcpy(root.userID, "root");
        strcpy(root.password, "sjtu");
        strcpy(root.username, "root");
        root.privilege = 7;
        accounts["root"] = root;
        saveAccounts();
    }
}

// Command handlers
void cmdSu(const vector<string>& params) {
    if (params.size() < 1 || params.size() > 2) {
        cout << "Invalid\n";
        return;
    }
    
    string userID = params[0];
    string password = params.size() == 2 ? params[1] : "";
    
    if (!isValidUserID(userID) || (params.size() == 2 && !isValidPassword(password))) {
        cout << "Invalid\n";
        return;
    }
    
    if (accounts.find(userID) == accounts.end()) {
        cout << "Invalid\n";
        return;
    }
    
    Account& acc = accounts[userID];
    
    // Check password requirement
    if (getCurrentPrivilege() <= acc.privilege) {
        if (params.size() != 2 || password != acc.password) {
            cout << "Invalid\n";
            return;
        }
    }
    
    LoginSession session;
    session.userID = userID;
    loginStack.push_back(session);
}

void cmdLogout() {
    if (getCurrentPrivilege() < 1) {
        cout << "Invalid\n";
        return;
    }
    
    loginStack.pop_back();
}

void cmdRegister(const vector<string>& params) {
    if (params.size() != 3) {
        cout << "Invalid\n";
        return;
    }

    string userID = params[0];
    string password = params[1];
    string username = params[2];

    if (!isValidUserID(userID) || !isValidPassword(password) || !isValidUsername(username)) {
        cout << "Invalid\n";
        return;
    }

    if (accounts.find(userID) != accounts.end()) {
        cout << "Invalid\n";
        return;
    }

    Account acc;
    strcpy(acc.userID, userID.c_str());
    strcpy(acc.password, password.c_str());
    strcpy(acc.username, username.c_str());
    acc.privilege = 1;
    accounts[userID] = acc;
    saveAccounts();
}

void cmdPasswd(const vector<string>& params) {
    if (getCurrentPrivilege() < 1) {
        cout << "Invalid\n";
        return;
    }

    if (params.size() < 2 || params.size() > 3) {
        cout << "Invalid\n";
        return;
    }

    string userID = params[0];
    string currentPassword = params.size() == 3 ? params[1] : "";
    string newPassword = params.size() == 3 ? params[2] : params[1];

    if (!isValidUserID(userID) || !isValidPassword(newPassword)) {
        cout << "Invalid\n";
        return;
    }

    if (params.size() == 3 && !isValidPassword(currentPassword)) {
        cout << "Invalid\n";
        return;
    }

    if (accounts.find(userID) == accounts.end()) {
        cout << "Invalid\n";
        return;
    }

    Account& acc = accounts[userID];

    if (getCurrentPrivilege() != 7) {
        if (params.size() != 3 || currentPassword != acc.password) {
            cout << "Invalid\n";
            return;
        }
    }

    strcpy(acc.password, newPassword.c_str());
    saveAccounts();
}

void cmdUseradd(const vector<string>& params) {
    if (getCurrentPrivilege() < 3) {
        cout << "Invalid\n";
        return;
    }

    if (params.size() != 4) {
        cout << "Invalid\n";
        return;
    }

    string userID = params[0];
    string password = params[1];
    string privilegeStr = params[2];
    string username = params[3];

    if (!isValidUserID(userID) || !isValidPassword(password) || !isValidUsername(username)) {
        cout << "Invalid\n";
        return;
    }

    if (privilegeStr.length() != 1 || !isdigit(privilegeStr[0])) {
        cout << "Invalid\n";
        return;
    }

    int privilege = privilegeStr[0] - '0';
    if (privilege != 1 && privilege != 3 && privilege != 7) {
        cout << "Invalid\n";
        return;
    }

    if (privilege >= getCurrentPrivilege()) {
        cout << "Invalid\n";
        return;
    }

    if (accounts.find(userID) != accounts.end()) {
        cout << "Invalid\n";
        return;
    }

    Account acc;
    strcpy(acc.userID, userID.c_str());
    strcpy(acc.password, password.c_str());
    strcpy(acc.username, username.c_str());
    acc.privilege = privilege;
    accounts[userID] = acc;
    saveAccounts();
}

void cmdDelete(const vector<string>& params) {
    if (getCurrentPrivilege() < 7) {
        cout << "Invalid\n";
        return;
    }

    if (params.size() != 1) {
        cout << "Invalid\n";
        return;
    }

    string userID = params[0];

    if (!isValidUserID(userID)) {
        cout << "Invalid\n";
        return;
    }

    if (accounts.find(userID) == accounts.end()) {
        cout << "Invalid\n";
        return;
    }

    // Check if user is logged in
    for (const auto& session : loginStack) {
        if (session.userID == userID) {
            cout << "Invalid\n";
            return;
        }
    }

    accounts.erase(userID);
    saveAccounts();
}

void cmdShow(const vector<string>& params) {
    if (getCurrentPrivilege() < 1) {
        cout << "Invalid\n";
        return;
    }

    vector<Book> results;

    if (params.empty()) {
        // Show all books
        for (const auto& pair : books) {
            results.push_back(pair.second);
        }
    } else if (params.size() == 1) {
        string param = params[0];
        if (param.substr(0, 6) == "-ISBN=") {
            string isbn = param.substr(6);
            if (isbn.empty() || !isValidISBN(isbn)) {
                cout << "Invalid\n";
                return;
            }
            if (books.find(isbn) != books.end()) {
                results.push_back(books[isbn]);
            }
        } else if (param.substr(0, 6) == "-name=") {
            if (param.length() < 9 || param[6] != '"' || param.back() != '"') {
                cout << "Invalid\n";
                return;
            }
            string name = param.substr(7, param.length() - 8);
            if (name.empty() || !isValidBookName(name)) {
                cout << "Invalid\n";
                return;
            }
            for (const auto& pair : books) {
                if (strcmp(pair.second.bookName, name.c_str()) == 0) {
                    results.push_back(pair.second);
                }
            }
        } else if (param.substr(0, 8) == "-author=") {
            if (param.length() < 11 || param[8] != '"' || param.back() != '"') {
                cout << "Invalid\n";
                return;
            }
            string author = param.substr(9, param.length() - 10);
            if (author.empty() || !isValidBookName(author)) {
                cout << "Invalid\n";
                return;
            }
            for (const auto& pair : books) {
                if (strcmp(pair.second.author, author.c_str()) == 0) {
                    results.push_back(pair.second);
                }
            }
        } else if (param.substr(0, 9) == "-keyword=") {
            if (param.length() < 12 || param[9] != '"' || param.back() != '"') {
                cout << "Invalid\n";
                return;
            }
            string keyword = param.substr(10, param.length() - 11);
            if (keyword.empty()) {
                cout << "Invalid\n";
                return;
            }
            // Check if keyword contains '|'
            if (keyword.find('|') != string::npos) {
                cout << "Invalid\n";
                return;
            }
            for (const auto& pair : books) {
                string keywords = pair.second.keyword;
                vector<string> kwList = split(keywords, '|');
                for (const auto& kw : kwList) {
                    if (kw == keyword) {
                        results.push_back(pair.second);
                        break;
                    }
                }
            }
        } else {
            cout << "Invalid\n";
            return;
        }
    } else {
        cout << "Invalid\n";
        return;
    }

    sort(results.begin(), results.end());

    if (results.empty()) {
        cout << "\n";
    } else {
        for (const auto& book : results) {
            cout << book.ISBN << "\t" << book.bookName << "\t" << book.author << "\t"
                 << book.keyword << "\t" << fixed << setprecision(2) << book.price << "\t"
                 << book.quantity << "\n";
        }
    }
}

void cmdBuy(const vector<string>& params) {
    if (getCurrentPrivilege() < 1) {
        cout << "Invalid\n";
        return;
    }

    if (params.size() != 2) {
        cout << "Invalid\n";
        return;
    }

    string isbn = params[0];
    string quantityStr = params[1];

    if (!isValidISBN(isbn) || !isValidQuantity(quantityStr)) {
        cout << "Invalid\n";
        return;
    }

    int quantity = stoi(quantityStr);

    if (books.find(isbn) == books.end()) {
        cout << "Invalid\n";
        return;
    }

    Book& book = books[isbn];

    if (book.quantity < quantity) {
        cout << "Invalid\n";
        return;
    }

    double totalCost = book.price * quantity;
    book.quantity -= quantity;

    Transaction trans;
    trans.amount = totalCost;
    trans.isIncome = true;
    transactions.push_back(trans);

    saveBooks();
    saveTransactions();

    cout << fixed << setprecision(2) << totalCost << "\n";
}

void cmdSelect(const vector<string>& params) {
    if (getCurrentPrivilege() < 3) {
        cout << "Invalid\n";
        return;
    }

    if (params.size() != 1) {
        cout << "Invalid\n";
        return;
    }

    string isbn = params[0];

    if (!isValidISBN(isbn)) {
        cout << "Invalid\n";
        return;
    }

    if (books.find(isbn) == books.end()) {
        // Create new book
        Book book;
        strcpy(book.ISBN, isbn.c_str());
        books[isbn] = book;
        saveBooks();
    }

    loginStack.back().selectedISBN = isbn;
}

void cmdModify(const vector<string>& params) {
    if (getCurrentPrivilege() < 3) {
        cout << "Invalid\n";
        return;
    }

    if (loginStack.empty() || loginStack.back().selectedISBN.empty()) {
        cout << "Invalid\n";
        return;
    }

    if (params.empty()) {
        cout << "Invalid\n";
        return;
    }

    string selectedISBN = loginStack.back().selectedISBN;
    Book book = books[selectedISBN];  // Make a copy instead of reference

    set<string> usedParams;
    string newISBN = "";

    // Debug output
    // cerr << "Modify called with " << params.size() << " params" << endl;
    // for (const auto& p : params) {
    //     cerr << "  param: [" << p << "]" << endl;
    // }

    for (const auto& param : params) {
        if (param.substr(0, 6) == "-ISBN=") {
            if (usedParams.count("-ISBN")) {
                cout << "Invalid\n";
                return;
            }
            usedParams.insert("-ISBN");
            newISBN = param.substr(6);
            if (newISBN.empty() || !isValidISBN(newISBN)) {
                cout << "Invalid\n";
                return;
            }
            if (newISBN == selectedISBN) {
                cout << "Invalid\n";
                return;
            }
            if (books.find(newISBN) != books.end()) {
                cout << "Invalid\n";
                return;
            }
        } else if (param.substr(0, 6) == "-name=") {
            if (usedParams.count("-name")) {
                cout << "Invalid\n";
                return;
            }
            usedParams.insert("-name");
            if (param.length() < 9 || param[6] != '"' || param.back() != '"') {
                cout << "Invalid\n";
                return;
            }
            string name = param.substr(7, param.length() - 8);
            if (name.empty() || !isValidBookName(name)) {
                cout << "Invalid\n";
                return;
            }
            strcpy(book.bookName, name.c_str());
        } else if (param.substr(0, 8) == "-author=") {
            if (usedParams.count("-author")) {
                cout << "Invalid\n";
                return;
            }
            usedParams.insert("-author");
            if (param.length() < 11 || param[8] != '"' || param.back() != '"') {
                cout << "Invalid\n";
                return;
            }
            string author = param.substr(9, param.length() - 10);
            if (author.empty() || !isValidBookName(author)) {
                cout << "Invalid\n";
                return;
            }
            strcpy(book.author, author.c_str());
        } else if (param.substr(0, 9) == "-keyword=") {
            if (usedParams.count("-keyword")) {
                cout << "Invalid\n";
                return;
            }
            usedParams.insert("-keyword");
            if (param.length() < 12 || param[9] != '"' || param.back() != '"') {
                cout << "Invalid\n";
                return;
            }
            string keyword = param.substr(10, param.length() - 11);
            if (keyword.empty() || !isValidKeyword(keyword)) {
                cout << "Invalid\n";
                return;
            }
            strcpy(book.keyword, keyword.c_str());
        } else if (param.substr(0, 7) == "-price=") {
            if (usedParams.count("-price")) {
                cout << "Invalid\n";
                return;
            }
            usedParams.insert("-price");
            string priceStr = param.substr(7);
            if (!isValidPrice(priceStr)) {
                cout << "Invalid\n";
                return;
            }
            book.price = stod(priceStr);
        } else {
            cout << "Invalid\n";
            return;
        }
    }

    if (!newISBN.empty()) {
        books.erase(selectedISBN);
        strcpy(book.ISBN, newISBN.c_str());
        books[newISBN] = book;
        loginStack.back().selectedISBN = newISBN;
    } else {
        // Update the book in place if ISBN wasn't changed
        books[selectedISBN] = book;
    }

    saveBooks();
}

void cmdImport(const vector<string>& params) {
    if (getCurrentPrivilege() < 3) {
        cout << "Invalid\n";
        return;
    }

    if (loginStack.empty() || loginStack.back().selectedISBN.empty()) {
        cout << "Invalid\n";
        return;
    }

    if (params.size() != 2) {
        cout << "Invalid\n";
        return;
    }

    string quantityStr = params[0];
    string totalCostStr = params[1];

    if (!isValidQuantity(quantityStr) || !isValidPrice(totalCostStr)) {
        cout << "Invalid\n";
        return;
    }

    int quantity = stoi(quantityStr);
    double totalCost = stod(totalCostStr);

    if (totalCost <= 0) {
        cout << "Invalid\n";
        return;
    }

    string selectedISBN = loginStack.back().selectedISBN;
    Book& book = books[selectedISBN];
    book.quantity += quantity;

    Transaction trans;
    trans.amount = totalCost;
    trans.isIncome = false;
    transactions.push_back(trans);

    saveBooks();
    saveTransactions();
}

void cmdShowFinance(const vector<string>& params) {
    if (getCurrentPrivilege() < 7) {
        cout << "Invalid\n";
        return;
    }

    if (params.size() > 1) {
        cout << "Invalid\n";
        return;
    }

    int count = transactions.size();

    if (params.size() == 1) {
        string countStr = params[0];
        if (countStr.empty() || countStr.length() > 10) {
            cout << "Invalid\n";
            return;
        }
        for (char c : countStr) {
            if (!isdigit(c)) {
                cout << "Invalid\n";
                return;
            }
        }

        // Use long long to avoid overflow
        long long countLL = stoll(countStr);
        if (countLL > 2147483647) {
            cout << "Invalid\n";
            return;
        }
        count = (int)countLL;

        if (count == 0) {
            cout << "\n";
            return;
        }

        if (count > (int)transactions.size()) {
            cout << "Invalid\n";
            return;
        }
    }

    double income = 0.0;
    double expenditure = 0.0;

    int start = transactions.size() - count;
    for (int i = start; i < (int)transactions.size(); i++) {
        if (transactions[i].isIncome) {
            income += transactions[i].amount;
        } else {
            expenditure += transactions[i].amount;
        }
    }

    cout << "+ " << fixed << setprecision(2) << income << " - " << expenditure << "\n";
}

void cmdLog() {
    if (getCurrentPrivilege() < 7) {
        cout << "Invalid\n";
        return;
    }

    cout << "=== System Log ===\n";
    cout << "Total transactions: " << transactions.size() << "\n";
}

void cmdReportFinance() {
    if (getCurrentPrivilege() < 7) {
        cout << "Invalid\n";
        return;
    }

    cout << "=== Financial Report ===\n";
    double income = 0.0;
    double expenditure = 0.0;

    for (const auto& trans : transactions) {
        if (trans.isIncome) {
            income += trans.amount;
        } else {
            expenditure += trans.amount;
        }
    }

    cout << "Total Income: " << fixed << setprecision(2) << income << "\n";
    cout << "Total Expenditure: " << expenditure << "\n";
    cout << "Net Profit: " << (income - expenditure) << "\n";
}

void cmdReportEmployee() {
    if (getCurrentPrivilege() < 7) {
        cout << "Invalid\n";
        return;
    }

    cout << "=== Employee Work Report ===\n";
    cout << "Total employees: " << accounts.size() << "\n";
}

int main() {
    initialize();

    string line;
    while (getline(cin, line)) {
        trim(line);

        if (line.empty()) {
            continue;
        }

        // Parse command
        vector<string> parts;
        string current = "";
        bool inQuotes = false;

        for (size_t i = 0; i < line.length(); i++) {
            char c = line[i];

            if (c == '"') {
                inQuotes = !inQuotes;
                current += c;
            } else if (c == ' ' && !inQuotes) {
                if (!current.empty()) {
                    parts.push_back(current);
                    current = "";
                }
            } else {
                current += c;
            }
        }

        if (!current.empty()) {
            parts.push_back(current);
        }

        if (parts.empty()) {
            continue;
        }

        string cmd = parts[0];
        vector<string> params(parts.begin() + 1, parts.end());

        if (cmd == "quit" || cmd == "exit") {
            break;
        } else if (cmd == "su") {
            cmdSu(params);
        } else if (cmd == "logout") {
            cmdLogout();
        } else if (cmd == "register") {
            cmdRegister(params);
        } else if (cmd == "passwd") {
            cmdPasswd(params);
        } else if (cmd == "useradd") {
            cmdUseradd(params);
        } else if (cmd == "delete") {
            cmdDelete(params);
        } else if (cmd == "show") {
            if (!params.empty() && params[0] == "finance") {
                cmdShowFinance(vector<string>(params.begin() + 1, params.end()));
            } else {
                cmdShow(params);
            }
        } else if (cmd == "buy") {
            cmdBuy(params);
        } else if (cmd == "select") {
            cmdSelect(params);
        } else if (cmd == "modify") {
            cmdModify(params);
        } else if (cmd == "import") {
            cmdImport(params);
        } else if (cmd == "log") {
            cmdLog();
        } else if (cmd == "report") {
            if (params.size() == 1) {
                if (params[0] == "finance") {
                    cmdReportFinance();
                } else if (params[0] == "employee") {
                    cmdReportEmployee();
                } else {
                    cout << "Invalid\n";
                }
            } else {
                cout << "Invalid\n";
            }
        } else {
            cout << "Invalid\n";
        }
    }

    return 0;
}

