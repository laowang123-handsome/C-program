#include "executor.hpp"
#include <filesystem>
#include <iostream>

static int tests_run = 0;
static int tests_failed = 0;

static void check(bool cond, const std::string& name) {
    ++tests_run;
    if (!cond) {
        ++tests_failed;
        std::cerr << "[FAIL] " << name << std::endl;
    } else {
        std::cout << "[PASS] " << name << std::endl;
    }
}

int main() {
    std::filesystem::remove_all("testdata");
    database db("testdata");

    QueryResult r;
    r = db.execute("create database school");
    check(r.ok, "create database");

    r = db.execute("use school");
    check(r.ok && r.message == "Database changed", "use database");

    r = db.execute("create table person (id int primary, name string)");
    check(r.ok, "create table with primary index");

    r = db.execute("insert person values (1001, \"peter\")");
    check(r.ok && r.affected == 1, "insert row 1");

    r = db.execute("insert person values (1002, \"mary\")");
    check(r.ok && r.affected == 1, "insert row 2");

    r = db.execute("insert person values (1001, \"duplicate\")");
    check(!r.ok, "reject duplicate primary key");

    r = db.execute("select name from person where id = 1001");
    check(r.ok && r.rows.size() == 1 && r.rows[0].values[0] == "peter", "select by primary key uses index");

    r = db.execute("select * from person where id > 1000");
    check(r.ok && r.rows.size() == 2, "select range by primary key");

    r = db.execute("update person set name = \"tom\" where id = 1001");
    check(r.ok && r.affected == 1, "update row");

    r = db.execute("select name from person where id = 1001");
    check(r.ok && r.rows.size() == 1 && r.rows[0].values[0] == "tom", "select after update");

    r = db.execute("delete person where id = 1002");
    check(r.ok && r.affected == 1, "delete row");

    r = db.execute("select * from person");
    check(r.ok && r.rows.size() == 1, "select after delete");

    r = db.execute("drop table person");
    check(r.ok, "drop table");

    r = db.execute("drop database school");
    check(r.ok, "drop database");

    std::filesystem::remove_all("testdata");
    std::cout << tests_run << " tests run, " << tests_failed << " failed." << std::endl;
    return tests_failed == 0 ? 0 : 1;
}
