#include "database.hpp"

#include <exception>

namespace skye_benchmark {

SQLiteContext::SQLiteContext(const std::string& filename)
    : connection_{MakeConnection(filename)}, statement_{MakeStatement(
                                                 connection_.get())}
{
}

std::optional<World> SQLiteContext::getRandomModel()
{
    constexpr int kNumColumn = 2;

    std::optional<World> model;

    auto* stmt = statement_.get();

    if ((sqlite3_step(stmt) == SQLITE_ROW) &&
        (sqlite3_column_count(stmt) == kNumColumn) &&
        (sqlite3_column_type(stmt, 0) == SQLITE_INTEGER) &&
        (sqlite3_column_type(stmt, 1) == SQLITE_INTEGER)) {
        model = {sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1)};
    }

    if (sqlite3_reset(stmt) != SQLITE_OK) {
        model.reset();
    }

    return model;
}

void SQLiteDeleter::operator()(sqlite3* ptr) const
{
    sqlite3_close(ptr);
}

void SQLiteDeleter::operator()(sqlite3_stmt* ptr) const
{
    sqlite3_finalize(ptr);
}

SQLiteContext::UniqueConnection
SQLiteContext::MakeConnection(const std::string& filename)
{
    // Change the following defaults:
    // - Do not create the database if it does not exist
    // - Do not use mutex internally, we are single threaded or will
    //   handle our own locking
    constexpr int kFlags = SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX;

    sqlite3* ptr = nullptr;

    const int ec = sqlite3_open_v2(filename.c_str(), &ptr, kFlags, nullptr);

    // From the SQLite docs:
    //
    // If the database is opened (and/or created) successfully, then
    // SQLITE_OK is returned. Otherwise an error code is returned.
    //
    // ...
    //
    // Whether or not an error occurs when it is opened, resources
    // associated with the database connection handle should be released by
    // passing it to sqlite3_close() when it is no longer required.
    //
    // https://www.sqlite.org/capi3ref.html#sqlite3_open
    UniqueConnection instance{ptr};

    if (ec != SQLITE_OK) {
        throw std::logic_error{sqlite3_errstr(ec)};
    }

    return instance;
}

SQLiteContext::UniqueStatement SQLiteContext::MakeStatement(sqlite3* db)
{
    constexpr std::string_view kSql{
        "SELECT * FROM world ORDER BY RANDOM() LIMIT 1;"};

    sqlite3_stmt* ptr = nullptr;

    const int ec =
        sqlite3_prepare_v2(db, kSql.data(), kSql.size(), &ptr, nullptr);

    UniqueStatement instance{ptr};

    if (ec != SQLITE_OK) {
        throw std::logic_error{sqlite3_errstr(ec)};
    }

    return instance;
}

} // namespace skye_benchmark