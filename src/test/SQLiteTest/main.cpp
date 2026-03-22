#include <iostream>
#include <sqlite3.h>
#include <string>

struct Student {
    std::string name;
    std::string gender;
    int age;
    double gap;

    Student(const std::string &n, const std::string &g, int a, double gp) : name(n), gender(g), age(a), gap(gp) {}
};

class StudentDB {
public:
    StudentDB(const std::string &dbName) {
        // 打开数据库
        int rc = sqlite3_open(dbName.c_str(), &_db);
        if (rc != SQLITE_OK) {
            std::cerr << "Error opening database: " << sqlite3_errmsg(_db)
                      << std::endl;
            sqlite3_close(_db);
        }

        // 创建表
        if (!initDataBase()) {
            sqlite3_close(_db);
        }
    }

    ~StudentDB() {
        if (_db) {
            sqlite3_close(_db);
        }
    }

    bool insertStudentInfo(const Student &s) {
        // 插入学生信息
        std::string sql = R"(
            INSERT INTO students (name, gender, age, gap)
            VALUES (?, ?, ?, ?);
        )";

        // 绑定参数
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(_db)
                      << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, s.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, s.gender.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, s.age);
        sqlite3_bind_double(stmt, 4, s.gap);

        // 执行 SQL 语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Error inserting student info: " << sqlite3_errmsg(_db)
                      << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt); // 清理资源避免内存泄漏
        return true;
    }

    // 查询学生信息
    bool queryAllStudents(const std::string &name) {
        std::string sql = R"(
            SELECT name, gender, age, gap FROM students WHERE name = ?;
        )";

        // 绑定参数
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(_db)
                      << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

        bool found = false;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            found = true;
            // 提取查询结果
            std::string queryName =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            std::string queryGender =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            int queryAge = sqlite3_column_int(stmt, 2);
            double queryGap = sqlite3_column_double(stmt, 3);

            // 打印结果
            std::cout << "查询结果: " << queryName << ", " << queryGender << ", "
                      << queryAge << ", " << queryGap << std::endl;
        }

        if (!found) {
            std::cout << "未找到学生: " << name << std::endl;
        }

        sqlite3_finalize(stmt); // 清理资源避免内存泄漏
        return true;
    }

    // 修改学生信息
    bool updateStudentInfo(const std::string& name, const Student& s) {
        std::string sql = R"( 
            UPDATE students SET gender = ?, age = ?, gap = ? WHERE name = ?;
        )";

        //准备SQL语句
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(_db)
                      << std::endl;
            return false;
        }

        //绑定参数
        sqlite3_bind_text(stmt, 1, s.gender.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, s.age);
        sqlite3_bind_double(stmt, 3, s.gap);
        sqlite3_bind_text(stmt, 4, name.c_str(), -1, SQLITE_TRANSIENT);

        //执行SQL语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Error updating student info: " << sqlite3_errmsg(_db)
                      << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt); // 清理资源避免内存泄漏
        return true;
    }

    //删除学生信息
    bool deleteStudentInfo(const std::string& name) {
        std::string sql = R"(
            DELETE FROM students WHERE name = ?;
        )";

        //准备SQL语句
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(_db)
                      << std::endl;
            return false;
        }

        //绑定参数
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

        //执行SQL语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Error deleting student info: " << sqlite3_errmsg(_db)
                      << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt); // 清理资源避免内存泄漏
        return true;
    }
private:
    bool initDataBase() {
        const char *sql = R"(
            CREATE TABLE IF NOT EXISTS students (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                gender TEXT NOT NULL,
                age INTEGER NOT NULL,
                gap REAL NOT NULL
            );
        )";

        char *errMsg = nullptr;
        int rc = sqlite3_exec(_db, sql, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "Error creating table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }

    sqlite3 *_db;
};

int main()
{
    Student s1("张三", "男", 18, 90.5);
    Student s2("李四", "女", 19, 85.0);
    Student s3("王五", "男", 20, 95.0);
    Student s4("赵六", "女", 21, 88.0);

    StudentDB db("student.db");
    db.insertStudentInfo(s1);
    db.insertStudentInfo(s2);
    db.insertStudentInfo(s3);
    db.insertStudentInfo(s4);
    
    //查询所有学生信息
    db.queryAllStudents("张三");

    // //修改学生信息
    // db.updateStudentInfo("张三", Student("男", 18, 90.5));
    // //查询所有学生信息
    // db.queryAllStudents("张三");
    
    //删除学生信息
    db.deleteStudentInfo("张三");
    //查询所有学生信息
    db.queryAllStudents("张三");
    return 0;
}