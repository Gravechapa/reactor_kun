#include "BotDB.hpp"
#include <inttypes.h>
#include "ReactorParser.hpp"
#include <thread>
#include <utility>
#include <iostream>
#include <stdexcept>
#include <sqlite3.h>

class Connection
{
public:
    Connection(std::string path, int flags)
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

    int isChanged() const noexcept
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
    PreparedStatment(Connection &db, std::string_view zSql)
    {
        if (zSql.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        {
            throw std::runtime_error("Sql query is too large");
        }
        int counter = 0;
        while (sqlite3_prepare_v2(db._connection, zSql.data(), static_cast<int>(zSql.size()),
                                  &_statment, nullptr) != SQLITE_OK)
        {
            if (++counter > 10)
            {
                throw std::runtime_error("Can't create PreparedStatment");
            }
            std::cout << "Can't create PreparedStatment, retrying: " << counter << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
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

    void bindText(int pos, std::string_view value)
    {
        if (sqlite3_bind_text64(_statment, pos, value.data(), value.size(),
                                SQLITE_TRANSIENT, SQLITE_UTF8) != SQLITE_OK)
        {
            throw std::runtime_error("Can't bind text to PreparedStatment");
        }
    }

    void bindNull(int pos)
    {
        if (sqlite3_bind_null(_statment, pos) != SQLITE_OK)
        {
            throw std::runtime_error("Can't bind null to PreparedStatment");
        }
    }

    bool next()
    {
        if (_afterLast)
        {
            return false;
        }
        int res;
        while ((res = sqlite3_step(_statment)) == SQLITE_BUSY)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(16ms);
        }
        if (res != SQLITE_ROW && res != SQLITE_DONE)
        {
           throw std::runtime_error(sqlite3_errmsg(_connection));
        }
        if (_beforeFirst)
        {
            _beforeFirst = false;
        }
        if (res == SQLITE_DONE)
        {
            _afterLast = true;
            return false;
        }
        return true;
    }

    void execute()
    {
        while (next());
    }

    int getInt(int col) const noexcept
    {
        return sqlite3_column_int(_statment, col);
    }

    sqlite3_int64 getInt64(int col) const noexcept
    {
        return sqlite3_column_int64(_statment, col);
    }

    std::string getText(int col) const noexcept
    {
        auto text = reinterpret_cast<const char*>(sqlite3_column_text(_statment, col));
        if (text)
        {
            return std::string(text);
        }
        return std::string();
    }

    bool isBeforeFirst() const noexcept
    {
        return _beforeFirst;
    }

    bool isAfterLast() const noexcept
    {
        return _afterLast;
    }

private:
    sqlite3_stmt *_statment = nullptr;
    sqlite3 *_connection;
    bool _beforeFirst = true;
    bool _afterLast = false;

};


BotDB::BotDB(std::string_view path)
{
    _path = path;
    if(sqlite3_threadsafe() == 0)
    {
        throw std::runtime_error("Yours sqlite don't have multithread support");
    }
    Connection connection(std::string(path), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX);

    {
        PreparedStatment stmt(connection, "CREATE TABLE IF NOT EXISTS listeners (ID INTEGER PRIMARY KEY NOT NULL," \
                                          " USERNAME TEXT NULL," \
                                          " FIRST_NAME TEXT NULL," \
                                          " LAST_NAME TEXT NULL);");
        stmt.execute();
    }

    {
        PreparedStatment stmt(connection, "CREATE TABLE IF NOT EXISTS reactor_data (ID INTEGER NOT NULL," \
                                          " TYPE INTEGER NOT NULL," \
                                          " TEXT TEXT NULL," \
                                          " DATA TEXT NULL);");
        stmt.execute();
    }

    PreparedStatment stmt(connection, "CREATE TABLE IF NOT EXISTS reactor_urls (ID INTEGER PRIMARY KEY NOT NULL," \
                                      " URL TEXT NOT NULL," \
                                      " TAGS TEXT NOT NULL," \
                                      " SENT INTEGER NOT NULL);");
    stmt.execute();

}

bool BotDB::newListener(int64_t id, std::string_view username,
                        std::string_view firstName, std::string_view lastName)
{
    Connection connection(_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

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
    Connection connection(_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection, "DELETE FROM listeners WHERE ID = ?;");

    stmt.bindInt64(1, id);

    stmt.execute();

    return connection.isChanged() != 0;
}

std::vector<int64_t> BotDB::getListeners()
{
    Connection connection(_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

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
    Connection connection(_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

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

bool BotDB::newReactorUrl(int64_t id, std::string_view url, std::string_view tags)
{
    Connection connection(_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection, "INSERT OR IGNORE INTO reactor_urls (ID, URL, TAGS, SENT) VALUES (?, ?, ?, 0);");

    stmt.bindInt64(1, id);
    stmt.bindText(2, url);
    stmt.bindText(3, tags);

    stmt.execute();

    return connection.isChanged() != 0;
}

bool BotDB::newReactorData(int64_t id, ElementType type, std::string_view text, const char* data)
{
    Connection connection(_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection, "INSERT OR FAIL INTO reactor_data (ID, TYPE, TEXT, DATA) VALUES (?, ?, ?, ?);");

    stmt.bindInt64(1, id);
    stmt.bindInt(2, static_cast<int>(type));
    text.empty() ? stmt.bindNull(3) : stmt.bindText(3, text);
    if (data)
    {
        stmt.bindText(4, data);
    }
    else
    {
        stmt.bindNull(4);
    }

    stmt.execute();

    return connection.isChanged() != 0;
}

void BotDB::markReactorPostsAsSent()
{
    Connection connection(_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection, "UPDATE OR IGNORE reactor_urls SET SENT = 1 WHERE SENT = 0;");
    stmt.execute();
}

std::vector<ReactorPost> BotDB::getNotSentReactorPosts()
{
    std::vector<ReactorPost> result;
    Connection connection(_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment resultSetUrls(connection, "SELECT ID, URL, TAGS FROM reactor_urls WHERE SENT = 0;");
    PreparedStatment resultSetData(connection, "SELECT * FROM reactor_data WHERE ID IN" \
                                   "(SELECT ID FROM reactor_urls WHERE SENT = 0) order by ID;");

      while(resultSetUrls.next())
     {
         result.push_back(_createReactorPost(resultSetUrls, resultSetData));
     }
     return result;
}

ReactorPost BotDB::getLatestReactorPost()
{
    ReactorPost result;
    Connection connection(_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment resultSetUrls(connection, "SELECT ID, URL, TAGS FROM reactor_urls order by ROWID DESC limit 1;");
    PreparedStatment resultSetData(connection, "SELECT * FROM reactor_data WHERE ID IN" \
                                   "(SELECT ID FROM reactor_urls order by ROWID DESC limit 1) order by ID;");

     if(resultSetUrls.next())
     {
         result = _createReactorPost(resultSetUrls, resultSetData);
     }
     return result;
}

ReactorPost BotDB::_createReactorPost(PreparedStatment &resultSetUrls, PreparedStatment &resultSetData)
{
    int64_t id = resultSetUrls.getInt64(0);
    ReactorPost post(resultSetUrls.getText(1), resultSetUrls.getText(2));

    if (resultSetData.isBeforeFirst())
    {
        resultSetData.next();
    }
    if (!resultSetData.isAfterLast())
    {
        do
        {
            if (id != resultSetData.getInt64(0))
            {
                return post;
            }
            post.emplaceElement(std::unique_ptr<RawElement>(new RawElement(id,
                                               static_cast<ElementType>(resultSetData.getInt(1)),
                                               resultSetData.getText(2),
                                               resultSetData.getText(3))));
        }
        while (resultSetData.next());
    }
    return post;
}

bool BotDB::empty()
{
    Connection connection(_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    PreparedStatment stmt(connection, "SELECT count(*) FROM reactor_urls;");
    stmt.next();
    return !stmt.getInt(0);
}

BotDB& BotDB::getBotDB()
{
    static BotDB botDB("./reactor_kun.db");
    return botDB;
}
