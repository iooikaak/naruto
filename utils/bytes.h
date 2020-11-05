//
// Created by 王振奎 on 2020/8/13.
//

#ifndef NARUTO__BYTES_H
#define NARUTO__BYTES_H

#define BB_DEFAULT_SIZE 4096

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <memory>
#include <iostream>
#include <cstdio>
#include <google/protobuf/message.h>
#include <glog/logging.h>

#include "protocol/message_type.h"

namespace naruto::utils{

class Bytes{

public:
    Bytes(uint32_t size = BB_DEFAULT_SIZE);
    Bytes(uint8_t* arr, uint32_t size);

    Bytes(Bytes&&) noexcept;
    Bytes& operator=(Bytes&&) noexcept;

    ~Bytes() = default;

    // 从已读位置开始剩余长度
    // 即还有多少未读
    uint32_t bytesRemaining();

    // 清空，重置 已读 和 写 的位置信息
    void clear();

    // 深拷贝，已读或已写的位置信息也一并copy过去
    std::unique_ptr<Bytes> clone();

    //Bytes compact(); // TODO?

    // 比较内容是否相等
    bool equals(Bytes* other);

    // 重置大小
    // 如果resize 前内容大于 newSize 则会清空多余的数据
    void resize(uint32_t newSize);

    // uint8_t size
    uint32_t size();

    // Basic Searching (Linear)
    template<typename T> int32_t find(T key, uint32_t start = 0)
    {
        int32_t ret = -1;
        uint32_t len = buf.size();
        for (uint32_t i = start; i < len; i++)
        {
            T data = read<T>(i);
            // Wasn't actually found, bounds of buffer were exceeded
            if ((key != 0) && (data == 0))
                break;

            // Key was found in array
            if (data == key)
            {
                ret = (int32_t)i;
                break;
            }
        }
        return ret;
    }

    // Replacement
    void replace(uint8_t key, uint8_t rep, uint32_t start = 0, bool firstOccuranceOnly = false);

    const unsigned char * data() const;

    // Read
    uint8_t peek() const; // just peek one uint8_t ，不会增加 pos

    void reset(int pos); // 重置 读 pos

    uint8_t get() const; // 获取下一个uint8_t 值
    uint8_t get(uint32_t index) const; // 获取下一个uint8_t 值，不移动 pos

    void getBytes(uint8_t* buf, uint32_t len) const; // Absolute read into array buf of length len

    char getChar() const; // Relative
    char getChar(uint32_t index) const; // Absolute

    double getDouble() const;
    double getDouble(uint32_t index) const;

    float getFloat() const;
    float getFloat(uint32_t index) const;

    uint32_t getInt() const;
    uint32_t getInt(uint32_t index) const;

    uint64_t getLong() const;
    uint64_t getLong(uint32_t index) const;

    uint16_t getShort() const;
    uint16_t getShort(uint32_t index) const;

    // Write

    void put(Bytes& src); // Relative write of the entire contents of another Bytes (src)
    void put(uint8_t b); // Relative write

    void put(uint8_t b, uint32_t index); // Absolute write at index

    void putBytes(uint8_t* b, uint32_t len); // Relative write
    void putBytes(uint8_t* b, uint32_t len, uint32_t index); // Absolute write starting at index

    void putChar(char value); // Relative
    void putChar(char value, uint32_t index); // Absolute

    void putDouble(double value);
    void putDouble(double value, uint32_t index);

    void putFloat(float value);
    void putFloat(float value, uint32_t index);

    void putInt(uint32_t value);
    void putInt(uint32_t value, uint32_t index);

    void putLong(uint64_t value);
    void putLong(uint64_t value, uint32_t index);

    void putShort(uint16_t value);
    void putShort(uint16_t value, uint32_t index);

    // 将message打成一个完成消息包
    void putMessage(const google::protobuf::Message& message, uint16_t type);

    // Buffer Position Accessors & Mutators

    void setReadPos(uint32_t r)
    {
        rpos = r;
    }

    uint32_t getReadPos() const
    {
        return rpos;
    }

    void setWritePos(uint32_t w)
    {
        wpos = w;
    }

    uint32_t getWritePos() const
    {
        return wpos;
    }

    void setName(std::string n);
    std::string getName();
    void printInfo();
    void printAH();
    void printAscii();
    void printHex();
    void printPosition();

private:
    uint32_t wpos;
    mutable uint32_t rpos;
    std::vector<uint8_t> buf;

    std::string name;

    template<typename T> T read() const
    {
        T data = read<T>(rpos);
        rpos += sizeof(T);
        return data;
    }

    template<typename T> T read(uint32_t index) const
    {
        if (index + sizeof(T) <= buf.size())
            return *((T*)&buf[index]);
        return 0;
    }

    template<typename T> void append(T data)
    {
        uint32_t s = sizeof(data);

        if (size() < (wpos + s))
            buf.resize(wpos + s);
        memcpy(&buf[wpos], (uint8_t*)&data, s);
        //printf("writing %c to %i\n", (uint8_t)data, wpos);

        wpos += s;
    }

    template<typename T> void insert(T data, uint32_t index)
    {
        if ((index + sizeof(data)) > size())
            return;

        memcpy(&buf[index], (uint8_t*)&data, sizeof(data));
        wpos = index + sizeof(data);
    }
};
}

#endif
