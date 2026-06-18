# MiniSQL 运行环境配置

## 操作系统

- 开发与测试系统：Linux
- 系统：Ubuntu 22.04 LTS 

## 编译环境

- C++ 标准：C++20
- 编译器：g++ 11 
- 构建工具：CMake 3.20 
- 网络：Linux POSIX Socket API，默认端口 3307

## 构建命令

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

## 运行命令

启动服务端：

```bash
./minisql_server 3307 ../data
```

另开一个终端启动客户端：

```bash
./minisql_client 127.0.0.1 3307
```

## 测试命令

```bash
./minisql_tests
```
