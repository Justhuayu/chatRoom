#!/bin/bash
# 1. 删除 ./build 下的所有文件
echo "Deleting files in ./build directory..."
rm -rf ./build/*

# 2. 在 ./build 目录下运行 cmake
echo "Running cmake in ./build directory..."
cd ./build
cmake ..

# 3. 在 ./build 目录下运行 make
echo "Running make in ./build directory..."
make

# 4. 执行 chatRoom 程序
echo "Executing chatRoom program..."
./chatRoomServer