#include "db.hpp"

std::mutex db_mutex;

int init_database()
{
    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return 1;
    }

    const char* sql_users
            = "CREATE TABLE IF NOT EXISTS users ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "login TEXT NOT NULL UNIQUE,"
              "password TEXT NOT NULL,"
              "online INTEGER NOT NULL DEFAULT 0);";

    char* err_msg = nullptr;
    rc = sqlite3_exec(db, sql_users, nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        std::cerr << "Can't create table users: " << err_msg << '\n';
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_free(err_msg);

    const char* sql_messages
            = "CREATE TABLE IF NOT EXISTS messages ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "is_file INTEGER NOT NULL DEFAULT 0,"
              "sender_id INTEGER NOT NULL,"
              "receiver_id INTEGER NOT NULL,"
              "text TEXT,"
              "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
              "FOREIGN KEY(sender_id) REFERENCES users(id),"
              "FOREIGN KEY(receiver_id) REFERENCES users(id));";

    rc = sqlite3_exec(db, sql_messages, nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        std::cerr << "Can't create table messages: " << err_msg << '\n';
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_free(err_msg);
    sqlite3_close(db);

    return 0;
}

bool user_exists(const std::string& login)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return false;
    }

    const char* sql = "SELECT COUNT(*) FROM users WHERE login = ?;";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    bool exists = false;
    if (rc == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    } else if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute query: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return exists;
}

int add_user(const std::string& login, const std::string& password)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return 1;
    }

    const char* sql = "INSERT INTO users (login, password) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to insert user: " << sqlite3_errmsg(db) << '\n';
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}

int get_user_id(const std::string& login)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return -1;
    }

    const char* sql = "SELECT id FROM users WHERE login = ?;";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int user_id = -1;

    if (rc == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
    } else if (rc == SQLITE_DONE) {
        std::cerr << "User \"" << login << "\" doesn't exist\n";
    } else {
        std::cerr << "Failed to execute query: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return user_id;
}

int add_message(const std::string& sender_login, const std::string& receiver_login, const std::string& message)
{
    int sender_id = get_user_id(sender_login);
    int receiver_id = get_user_id(receiver_login);

    if (sender_id == -1) {
        std::cerr << "Sender not found: " << sender_login << '\n';
        return 1;
    }
    if (receiver_id == -1) {
        std::cerr << "receiver not found: " << receiver_login << '\n';
        return 1;
    }

    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return 1;
    }

    const char* sql = "INSERT INTO messages (sender_id, receiver_id, text, timestamp) VALUES (?, ?, ?, datetime('now', '+7 hours'));";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_int(stmt, 1, sender_id);
    sqlite3_bind_int(stmt, 2, receiver_id);
    sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to insert message: " << sqlite3_errmsg(db) << '\n';
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}

int add_file(const std::string& sender_login, const std::string& receiver_login, const std::string& path)
{
    int sender_id = get_user_id(sender_login);
    int receiver_id = get_user_id(receiver_login);

    if (sender_id == -1) {
        std::cerr << "Sender not found: " << sender_login << '\n';
        return 1;
    }
    if (receiver_id == -1) {
        std::cerr << "receiver not found: " << receiver_login << '\n';
        return 1;
    }

    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return 1;
    }

    const char* sql = "INSERT INTO messages (is_file, sender_id, receiver_id, text) VALUES (1, ?, ?, ?);";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_int(stmt, 1, sender_id);
    sqlite3_bind_int(stmt, 2, receiver_id);
    sqlite3_bind_text(stmt, 3, path.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to add file: " << sqlite3_errmsg(db) << '\n';
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}

bool verify_user(const std::string& login, const std::string& password)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return false;
    }

    const char* sql = "SELECT id FROM users WHERE login = ? AND password = ?;";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    bool approved = false;
    if (rc == SQLITE_ROW) {
        approved = true;
    } else if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute query: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return approved;
}

int change_user_status(const std::string& login, int status)
{
    if (status != OFFLINE && status != ONLINE) {
        std::cerr << "Incorrect status value\n";
        return 1;
    }

    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return 1;
    }

    const char* sql = "UPDATE users SET online = ? WHERE login = ?;";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_text(stmt, 2, login.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to change status: " << sqlite3_errmsg(db) << '\n';
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}

int get_user_status(const std::string& login)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return -1;
    }

    const char* sql = "SELECT online FROM users WHERE login = ?;";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int user_status = -1;

    if (rc == SQLITE_ROW) {
        user_status = sqlite3_column_int(stmt, 0);
    } else if (rc == SQLITE_DONE) {
        std::cerr << "User \"" << login << "\" doesn't exist\n";
    } else {
        std::cerr << "Failed to execute query: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return user_status;
}

users_map get_users_map()
{
    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return {};
    }

    const char* sql = "SELECT login, online FROM users;";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return {};
    }

    users_map users;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int status = sqlite3_column_int(stmt, 1);
        if (login)
            users[login] = status;
        else
            std::cerr << "Found user with NULL login\n";
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute query: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return users;
}

messages_list get_messages_list(const std::string& sender_login, const std::string& receiver_login)
{
    int sender_id = get_user_id(sender_login);
    int receiver_id = get_user_id(receiver_login);

    if (sender_id == -1) {
        std::cerr << "Sender not found: " << sender_login << '\n';
        return {};
    }
    if (receiver_id == -1) {
        std::cerr << "receiver not found: " << receiver_login << '\n';
        return {};
    }

    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3* db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return {};
    }

    const char* sql
            = "SELECT timestamp, sender_id, text, is_file "
              "FROM messages "
              "WHERE (sender_id = ? AND receiver_id = ?) "
              "OR (sender_id = ? AND receiver_id = ?) "
              "ORDER BY timestamp ASC;";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return {};
    }

    sqlite3_bind_int(stmt, 1, sender_id);
    sqlite3_bind_int(stmt, 2, receiver_id);
    sqlite3_bind_int(stmt, 3, receiver_id);
    sqlite3_bind_int(stmt, 4, sender_id);

    messages_list messages;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string timestamp_text = timestamp ? timestamp : "";
        int this_message_sender_id = sqlite3_column_int(stmt, 1);
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string message_text = text ? text : "";
        int is_file = sqlite3_column_int(stmt, 3);

        if (this_message_sender_id == sender_id)
            messages.push_back(message{timestamp_text, sender_login, message_text, is_file});
        else
            messages.push_back(message{timestamp_text, receiver_login, message_text, is_file});
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute query: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return messages;
}
