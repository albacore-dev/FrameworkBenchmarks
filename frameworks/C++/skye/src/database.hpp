#include "model.hpp"

#include <sqlite3.h>

#include <memory>
#include <optional>
#include <random>
#include <string>

namespace skye_benchmark {

// Custom deleter for unique_ptr/shared_ptr to sqlite3 structs.
struct SQLiteDeleter {
    void operator()(sqlite3* ptr) const;
    void operator()(sqlite3_stmt* ptr) const;
};

/** Connection to the database. Use the SQLite C API. */
class SQLiteContext {
public:
    // RAII, requires that the database is open and the statement is prepared.
    explicit SQLiteContext(const std::string& filename);

    // Model not set if:
    // - No matching row in database
    // - Column types do not match object
    // - Error in executing statement
    std::optional<World> getRandomModel();

private:
    using UniqueConnection = std::unique_ptr<sqlite3, SQLiteDeleter>;
    using UniqueStatement = std::unique_ptr<sqlite3_stmt, SQLiteDeleter>;

    static UniqueConnection MakeConnection(const std::string& filename);
    static UniqueStatement MakeStatement(sqlite3* db);

    // Prepared statement depends on the connection.
    UniqueConnection connection_;
    UniqueStatement statement_;

    // Used to randomly select a row by its id field in `getRandomModel`.
    std::mt19937 engine_;
    std::uniform_int_distribution<int> uniform_dist_;
};

} // namespace skye_benchmark