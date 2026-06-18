# 四层实现路线对应说明

## 第一层：项目能跑

已实现：

- CMake 构建系统
- `minisql_server`
- `minisql_client`
- `minisql_tests`
- `config.md`
- TCP 客户端/服务端通信

相关文件：

- `CMakeLists.txt`
- `src/server_main.cpp`
- `src/client_main.cpp`
- `src/network.cpp`
- `include/network.hpp`

## 第二层：SQL 解析器

已实现 SQL 语法解析：

- `create database`
- `drop database`
- `use`
- `create table`
- `drop table`
- `insert`
- `select`
- `delete`
- `update`

相关文件：

- `include/sql_parser.hpp`
- `src/sql_parser.cpp`

## 第三层：文件存储

已实现：

- 数据库目录创建/删除
- 表结构文件 `.meta`
- 表数据文件 `.dat`
- 主键索引文件 `.idx`
- 表加载和保存

相关文件：

- `include/storage_engine.hpp`
- `src/storage_engine.cpp`

## 第四层：核心类

已实现任务书建议的核心类：

- `column`
- `row`
- `table`
- `index`：B+ 树主键索引
- `database`：执行引擎和当前数据库上下文

额外实现：

- `SimpleArray<T>`：自定义动态数组
- `SqlParser`：SQL 解析器
- `StorageEngine`：存储引擎
- `TcpServer` / `TcpClient`：网络层
- `ResultCodec`：结果集序列化/反序列化

相关文件：

- `include/schema.hpp`
- `include/simple_array.hpp`
- `include/bplustree.hpp`
- `include/executor.hpp`
- `include/result_codec.hpp`
- `include/network.hpp`
