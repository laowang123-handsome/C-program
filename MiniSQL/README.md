# MiniSQL：基于 C++20 的微型关系型数据库管理系统

本项目按照《C++现代程序设计》课程大作业要求，实现一个简化版 MySQL 风格的关系型数据库管理系统。

## 已实现功能

### 1. C/S 架构

- `minisql_server`：服务端，负责 SQL 解析、执行、文件存储和索引维护。
- `minisql_client`：客户端，提供 MySQL 风格 CLI，用户输入 SQL 后通过 TCP 发送给服务端。
- 通信方式：Linux TCP Socket。
- 返回结果：使用自定义文本协议序列化/反序列化结果集。

### 2. 文件存储

默认存储目录：`data/`

示例结构：

```text
data/
  school/
    person.meta
    person.dat
    person.idx
```

- `.meta`：表结构，包括列名、类型、是否主键。
- `.dat`：表数据。
- `.idx`：主键索引的有序导出文件。

### 3. DDL

支持：

```sql
create database <dbname>
drop database <dbname>
use <dbname>
create table <table-name> (<column> <type> [primary], ...)
drop table <table-name>
```

类型支持：

- `int`
- `string`，最长 256 字符

### 4. DML

支持：

```sql
select <column> from <table> [where <column> <op> <value>]
delete <table> [where <column> <op> <value>]
insert <table> values (<value>, ...)
update <table> set <column> = <value> [where <column> <op> <value>]
```

其中 `<op>` 支持：

```text
= < >
```

### 5. B+ 树索引

- 对 `primary` 列建立 B+ 树索引。
- `select` 条件命中主键列时使用索引。
- `insert` 检查主键唯一性。
- `delete` / `update` 后重建索引，保证索引一致性。

### 6. 自定义容器

为符合“不使用 STL 标准容器”的要求，项目实现了：

```cpp
SimpleArray<T>
```

代码中未使用 `std::vector`、`std::map`、`std::list`、`std::set`、`std::unordered_map` 等 STL 标准容器。

说明：本版本使用 `std::string` 作为基础字符串类型，使用 `iostream`、`fstream`、`filesystem`、`sstream` 等基础库完成输入输出和文件系统功能。

## 构建

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## 单元测试

```bash
ctest --output-on-failure
```

或：

```bash
./minisql_tests
```

## 运行示例

终端 1：

```bash
./minisql_server 3307 ../data
```

终端 2：

```bash
./minisql_client 127.0.0.1 3307
```

客户端输入：

```sql
create database school
use school
create table person (id int primary, name string)
insert person values (1001, "peter")
insert person values (1002, "mary")
select * from person
select name from person where id = 1001
update person set name = "tom" where id = 1001
delete person where id = 1002
exit
```

## 项目文件说明

```text
include/simple_array.hpp     自定义动态数组，替代 STL 容器
include/schema.hpp           column、row、table、Condition、QueryResult
include/bplustree.hpp        B+ 树索引类 index
include/sql_parser.hpp       SQL 解析器
include/storage_engine.hpp   文件存储引擎
include/executor.hpp         执行引擎 database
include/result_codec.hpp     结果集序列化/反序列化
include/network.hpp          TCP 服务端和客户端
src/*.cpp                    各模块实现
src/server_main.cpp          服务端入口
src/client_main.cpp          客户端入口
src/tests_main.cpp           单元测试入口
config.md                    运行环境配置
CMakeLists.txt               CMake 构建脚本
```
