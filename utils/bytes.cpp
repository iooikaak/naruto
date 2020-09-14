//
// Created by 王振奎 on 2020/8/13.
//

#include "bytes.h"

namespace naruto {
namespace utils{

Bytes::Bytes(uint32_t size) {
    buf.reserve(size);
    clear();
    name = "";
}

Bytes::Bytes(uint8_t* arr, uint32_t size) {
    // If the provided array is NULL, allocate a blank buffer of the provided size
    if (arr == NULL) {
        buf.reserve(size);
        clear();
    } else { // Consume the provided array
        buf.reserve(size);
        clear();
        putBytes(arr, size);
    }
    name = "";
}

Bytes::Bytes(Bytes && in) noexcept {
    this->buf = std::move(in.buf);
    this->name = std::move(in.name);
    this->wpos = in.wpos;
    this->rpos = in.rpos;
}

Bytes& Bytes::operator=(Bytes && in) noexcept{
    this->buf = std::move(in.buf);
    this->name = std::move(in.name);
    this->wpos = in.wpos;
    this->rpos = in.rpos;
    return *this;
}

uint32_t Bytes::bytesRemaining() {
        return size() - rpos;
}

void Bytes::clear() {
    rpos = 0;
    wpos = 0;
    buf.clear();
}

std::unique_ptr<Bytes> Bytes::clone() {
    std::unique_ptr<Bytes> ret(new Bytes(buf.size()));

    // Copy data
    for (uint32_t i = 0; i < buf.size(); i++) {
        ret->put((uint8_t) get(i));
    }

    // Reset positions
    ret->setReadPos(0);
    ret->setWritePos(0);

    return ret;
}

bool Bytes::equals(Bytes* other) {
    // If sizes aren't equal, they can't be equal
    if (size() != other->size())
        return false;

    // Compare byte by byte
    uint32_t len = size();
    for (uint32_t i = 0; i < len; i++) {
        if ((uint8_t) get(i) != (uint8_t) other->get(i))
            return false;
    }

    return true;
}

void Bytes::resize(uint32_t newSize) {
    buf.resize(newSize);
    rpos = 0;
    wpos = 0;
}

uint32_t Bytes::size() {
    return buf.size();
}

void Bytes::replace(uint8_t key, uint8_t rep, uint32_t start, bool firstOccuranceOnly) {
    uint32_t len = buf.size();
    for (uint32_t i = start; i < len; i++) {
        uint8_t data = read<uint8_t>(i);
        // Wasn't actually found, bounds of buffer were exceeded
        if ((key != 0) && (data == 0))
            break;

        // Key was found in array, perform replacement
        if (data == key) {
            buf[i] = rep;
            if (firstOccuranceOnly)
                return;
        }
    }
}

unsigned char *Bytes::data() { return buf.data(); }

// Read Functions
uint8_t Bytes::peek() const {
    return read<uint8_t>(rpos);
}

void Bytes::reset(int pos){ rpos = pos; }

uint8_t Bytes::get() const {
    return read<uint8_t>();
}

uint8_t Bytes::get(uint32_t index) const {
    return read<uint8_t>(index);
}

void Bytes::getBytes(uint8_t* buf, uint32_t len) const {
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = read<uint8_t>();
    }
}

char Bytes::getChar() const {
    return read<char>();
}

char Bytes::getChar(uint32_t index) const {
    return read<char>(index);
}

double Bytes::getDouble() const {
    return read<double>();
}

double Bytes::getDouble(uint32_t index) const {
    return read<double>(index);
}

float Bytes::getFloat() const {
    return read<float>();
}

float Bytes::getFloat(uint32_t index) const {
    return read<float>(index);
}

uint32_t Bytes::getInt() const {
    return read<uint32_t>();
}

uint32_t Bytes::getInt(uint32_t index) const {
    return read<uint32_t>(index);
}

uint64_t Bytes::getLong() const {
    return read<uint64_t>();
}

uint64_t Bytes::getLong(uint32_t index) const {
    return read<uint64_t>(index);
}

uint16_t Bytes::getShort() const {
    return read<uint16_t>();
}

uint16_t Bytes::getShort(uint32_t index) const {
    return read<uint16_t>(index);
}

// Write Functions

void Bytes::put(Bytes* src) {
    uint32_t len = src->size();
    for (uint32_t i = 0; i < len; i++)
        append<uint8_t>(src->get(i));
}

void Bytes::put(uint8_t b) {
    append<uint8_t>(b);
}

void Bytes::put(uint8_t b, uint32_t index) {
    insert<uint8_t>(b, index);
}

void Bytes::putBytes(uint8_t* b, uint32_t len) {
    // Insert the data one byte at a time into the internal buffer at position i+starting index
    for (uint32_t i = 0; i < len; i++)
        append<uint8_t>(b[i]);
}

void Bytes::putBytes(uint8_t* b, uint32_t len, uint32_t index) {
    wpos = index;

    // Insert the data one byte at a time into the internal buffer at position i+starting index
    for (uint32_t i = 0; i < len; i++)
        append<uint8_t>(b[i]);
}

void Bytes::putChar(char value) {
    append<char>(value);
}

void Bytes::putChar(char value, uint32_t index) {
    insert<char>(value, index);
}

void Bytes::putDouble(double value) {
    append<double>(value);
}

void Bytes::putDouble(double value, uint32_t index) {
    insert<double>(value, index);
}
void Bytes::putFloat(float value) {
    append<float>(value);
}

void Bytes::putFloat(float value, uint32_t index) {
    insert<float>(value, index);
}

void Bytes::putInt(uint32_t value) {
    append<uint32_t>(value);
}

void Bytes::putInt(uint32_t value, uint32_t index) {
    insert<uint32_t>(value, index);
}

void Bytes::putLong(uint64_t value) {
    append<uint64_t>(value);
}

void Bytes::putLong(uint64_t value, uint32_t index) {
    insert<uint64_t>(value, index);
}

void Bytes::putShort(uint16_t value) {
    append<uint16_t>(value);
}

void Bytes::putShort(uint16_t value, uint32_t index) {
    insert<uint16_t>(value, index);
}

void Bytes::putMessage(const google::protobuf::Message &message, uint16_t type) {
    uint32_t body_size = message.ByteSizeLong();
    uint32_t pack_size = PACK_HEAD_LEN + body_size;
    // 包长度 4 字节
    putInt(pack_size);
    // 包版本号 1 字节
    uint8_t version = 1;
    put(version);
    uint8_t flag = 0;
    // 包 标志位 1 字节
    put(flag);
    // 包消息类型 2 字节
    putShort(type);

    LOG(INFO) << "putMessage pack_size:" << pack_size << " body size:" << body_size << " version:" << unsigned(version) << " flag:" << unsigned(flag) << std::endl;
    // body
    char buf[body_size];
    message.SerializeToArray(buf, body_size);
    putBytes((uint8_t*)buf, body_size);
}

void Bytes::setName(std::string n) {
    name = n;
}

std::string Bytes::getName() {
    return name;
}

void Bytes::printInfo() {
    uint32_t length = buf.size();
    std::cout << "Bytes " << name.c_str() << " Length: " << length << ". Info Print" << std::endl;
}

void Bytes::printAH() {
    uint32_t length = buf.size();
    std::cout << "Bytes " << name.c_str() << " Length: " << length << ". ASCII & Hex Print" << std::endl;

    for (uint32_t i = 0; i < length; i++) {
        std::printf("0x%02x ", buf[i]);
    }

    std::printf("\n");
    for (uint32_t i = 0; i < length; i++) {
        std::printf("%c ", buf[i]);
    }

    std::printf("\n");
}

void Bytes::printAscii() {
    uint32_t length = buf.size();
    std::cout << "Bytes " << name.c_str() << " Length: " << length << ". ASCII Print" << std::endl;

    for (uint32_t i = 0; i < length; i++) {
        std::printf("%c ", buf[i]);
    }

    std::printf("\n");
}

void Bytes::printHex() {
    uint32_t length = buf.size();
    std::cout << "Bytes " << name.c_str() << " Length: " << length << ". Hex Print" << std::endl;

    for (uint32_t i = 0; i < length; i++) {
        std::printf("0x%02x ", buf[i]);
    }

    std::printf("\n");
}

void Bytes::printPosition() {
    uint32_t length = buf.size();
    std::cout << "Bytes " << name.c_str() << " Length: " << length << " Read Pos: " << rpos << ". Write Pos: "
              << wpos << std::endl;
}

}
}