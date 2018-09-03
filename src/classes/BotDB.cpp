#include "BotDB.hpp"
#include <inttypes.h>

class PreparedStatment;

class Connection
{
public:
    Connection (std::string path, int flags)
    {
        if(sqlite3_open_v2(path.c_str(), &_connection, flags, nullptr) != SQLITE_OK)
            {
                throw std::runtime_error("Can't open/cteate db: " + path);
            }
    }
    ~Connection()
    {
        if (sqlite3_close_v2(_connection) != SQLITE_OK)
            {
                std::cerr << "PreparedStatment wasn't properly destroyed" << std::endl;
            }
    }

    int isChanged()
    {
        return sqlite3_changes(_connection);
    }

    friend PreparedStatment;

private:
    sqlite3 *_connection = nullptr;
};

class PreparedStatment
{
public:
    PreparedStatment(Connection &db, std::string zSql)
    {
        if (sqlite3_prepare_v2(db._connection, zSql.c_str(), -1, &_statment, nullptr) != SQLITE_OK)
            {
                throw std::runtime_error("Can't create PreparedStatment");
            }
        _connection = db._connection;
    }

    ~PreparedStatment()
    {
        if (sqlite3_finalize(_statment) != SQLITE_OK)
            {
                std::cerr << "PreparedStatment wasn't properly destroyed" << std::endl;
            }
    }

    void bindInt(int pos, int value)
    {
        if (sqlite3_bind_int(_statment, pos, value) != SQLITE_OK)
            {
                throw std::runtime_error("Can't bind int to PreparedStatment");
            }
    }

    void bindInt64(int pos, sqlite3_int64 value)
    {
        if (sqlite3_bind_int64(_statment, pos, value) != SQLITE_OK)
            {
                throw std::runtime_error("Can't bind int64 to PreparedStatment");
            }
    }

    void bindText(int pos, std::string &value)
    {
        if (sqlite3_bind_text(_statment, pos, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
            {
                throw std::runtime_error("Can't bind text to PreparedStatment");
            }
    }

    bool next()
    {
        auto res = sqlite3_step(_statment);
        if (res != SQLITE_ROW && res != SQLITE_DONE)
            {
               throw std::runtime_error(sqlite3_errmsg(_connection));
           }
        if (res == SQLITE_DONE)
            {
                return false;
            }
        return true;
    }

    void execute()
    {
        while (next());
    }

    int getInt(int col)
    {
        return sqlite3_column_int(_statment, col);
    }

    sqlite3_int64 getInt64(int col)
    {
        return sqlite3_column_int64(_statment, col);
    }

    std::string getText(int col)
    {
        return std::string(reinterpret_cast<const char*>(sqlite3_column_text(_statment, col)));
    }


private:
    sqlite3_stmt *_statment = nullptr;
    sqlite3 *_connection;

};


BotDB::BotDB(std::string path)
{
    _path = path;
    if(sqlite3_threadsafe() == 0)
        {
            throw std::runtime_error("Yours sqlite don't have multithread support");
        }
    Connection connection(path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX);

    std::string query = "CREATE TABLE IF NOT EXISTS listeners (ID INTEGER PRIMARY KEY NOT NULL," \
                                                   " USERNAME TEXT NULL," \
                                                   " FIRST_NAME TEXT NULL," \
                                                   " LAST_NAME TEXT NULL);";
    {
        PreparedStatment stmt(connection, query);
        stmt.execute();
    }

    query = "CREATE TABLE IF NOT EXISTS reactor_data (ID INTEGER NOT NULL," \
                                              " TYPE INTEGER NOT NULL," \
                                              " TEXT TEXT NULL," \
                                              " DATA TEXT NULL);";
    {
        PreparedStatment stmt(connection, query);
        stmt.execute();
    }

    query = "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'reactor_urls';";
    bool result;
    {
        PreparedStatment stmt(connection, query);
        result = stmt.next();
    }
    if(!result)
        {
            query = "CREATE TABLE reactor_urls (ID INTEGER PRIMARY KEY NOT NULL," \
                                                " URL TEXT NOT NULL," \
                                                " TAGS TEXT NOT NULL," \
                                                " SENT INTEGER NOT NULL);";
            PreparedStatment stmt(connection, query);
            stmt.execute();
            //TODO init values;
        }
}

bool BotDB::newListener(int64_t id, std::string username, std::string firstName, std::string lastName)
{
    Connection connection (_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection,
                          "INSERT OR IGNORE INTO listeners (ID, USERNAME, FIRST_NAME, LAST_NAME) VALUES (?, ?, ?, ?);");

    stmt.bindInt64(1, id);
    stmt.bindText(2, username);
    stmt.bindText(3, firstName);
    stmt.bindText(4, lastName);

    stmt.execute();

    return connection.isChanged() != 0;
}

bool BotDB::deleteListener(int64_t id)
{
    Connection connection (_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection, "DELETE FROM listeners where ID = ?;");

    stmt.bindInt64(1, id);

    stmt.execute();

    return connection.isChanged() != 0;
}

std::vector<int64_t> BotDB::getListeners()
{
    Connection connection (_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection, "SELECT ID FROM listeners;");
    std::vector<int64_t> result;
    while (stmt.next())
        {
            result.push_back(stmt.getInt64(0));
        }
    return result;
}

void BotDB::deleteOldReactorPosts(int limit)
{
    Connection connection (_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    int count;
    {
        PreparedStatment stmt(connection, "SELECT count(*) FROM reactor_urls;");
        stmt.next();
        count = stmt.getInt(0);
    }

    if (count > limit)
        {
            int forDelete = count - limit;
            {
                PreparedStatment stmt(connection,
                                      "DELETE FROM reactor_data WHERE ID IN (SELECT ID FROM reactor_urls order by ROWID limit ?);");
                stmt.bindInt(1, forDelete);
                stmt.execute();
            }
            PreparedStatment stmt(connection,
                                  "DELETE FROM reactor_urls WHERE ID IN " \
                                  "(SELECT ID FROM reactor_urls order by ROWID limit ?);");
            stmt.bindInt(1, forDelete);
            stmt.execute();
        }
}

bool BotDB::newReactorUrl(int64_t id, std::string url, std::string tags)
{
    Connection connection (_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection, "INSERT OR IGNORE INTO reactor_urls (ID, URL, TAGS, SENT) VALUES (?, ?, ?, 0);");

    stmt.bindInt64(1, id);
    stmt.bindText(2, url);
    stmt.bindText(3, tags);

    stmt.execute();

    return connection.isChanged() != 0;
}

bool BotDB::newReactorData(int64_t id, ElementType type, std::string text, std::string data)
{
    Connection connection (_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection, "INSERT OR FAIL INTO reactor_data (ID, TYPE, TEXT, DATA) VALUES (?, ?, ?, ?);");

    stmt.bindInt64(1, id);
    stmt.bindInt(2, static_cast<int>(type));
    stmt.bindText(3, text);
    stmt.bindText(4, data);

    stmt.execute();

    return connection.isChanged() != 0;
}

void BotDB::markReactorPostsAsSent()
{
    Connection connection (_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection, "UPDATE OR IGNORE reactor_urls SET SENT = 1 WHERE SENT = 0;");
    stmt.execute();
}

//TODO getNotSentReactorPosts
//getLatestReactorPost
//createReactorPost
