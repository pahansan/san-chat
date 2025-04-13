#include <sqlite3.h>
#include <iostream>

int init_database()
{
    sqlite3 *db;
    int rc = sqlite3_open("chat.db", &db);

    if (rc)
    {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        return 1;
    }

    const char *sql_users = "CREATE TABLE IF NOT EXISTS users ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "login TEXT NOT NULL UNIQUE,"
                            "password TEXT NOT NULL,"
                            "online INTEGER NOT NULL DEFAULT 0);";

    char *err_msg = nullptr;
    rc = sqlite3_exec(db, sql_users, nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Can't create table users: " << err_msg << '\n';
        sqlite3_free(err_msg);
        return 1;
    }

    sqlite3_free(err_msg);

    const char *sql_messages = "CREATE TABLE IF NOT EXISTS messages ("
                               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                               "is_file INTEGER NOT NULL DEFAULT 0,"
                               "sender_id INTEGER NOT NULL,"
                               "reciever_id INTEGER NOT NULL,"
                               "text TEXT,"
                               "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
                               "FOREIGN KEY(sender_id) REFERENCES users(id),"
                               "FOREIGN KEY(reciever_id) REFERENCES users(id));";

    rc = sqlite3_exec(db, sql_messages, nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Can't create table messages: " << err_msg << '\n';
        sqlite3_free(err_msg);
        return 1;
    }

    sqlite3_free(err_msg);
    sqlite3_close(db);

    return 0;
}

int main()
{
    return init_database();
}