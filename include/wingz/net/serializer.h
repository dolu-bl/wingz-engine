#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <wingz/net/types.h>

namespace wingz::net
{

/// Сериализатор/десериализатор в бинарный буфер (little-endian).
/// Пишет и читает примитивные типы побайтово, избегая UB выравнивания и endianness.
class Serializer
{
public:
    explicit Serializer(std::vector<uint8_t>& buffer)
        : m_buffer(buffer)
        , m_writeOffset(buffer.size())
    {
    }

    explicit Serializer(const std::vector<uint8_t>& buffer)
        : m_buffer(const_cast<std::vector<uint8_t>&>(buffer))
        , m_readOffset(0)
        , m_isReader(true)
    {
    }

    // Запрещаем копирование
    Serializer(const Serializer&) = delete;
    Serializer& operator=(const Serializer&) = delete;

    // ────────────────────────────────────────
    // Запись
    // ────────────────────────────────────────

    void writeU8(uint8_t value)
    {
        ensureWriteCapacity(1);
        m_buffer[m_writeOffset] = value;
        m_writeOffset += 1;
    }

    void writeU16(uint16_t value)
    {
        ensureWriteCapacity(2);
        m_buffer[m_writeOffset] = static_cast<uint8_t>(value & 0xFF);
        m_buffer[m_writeOffset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        m_writeOffset += 2;
    }

    void writeU32(uint32_t value)
    {
        ensureWriteCapacity(4);
        m_buffer[m_writeOffset] = static_cast<uint8_t>(value & 0xFF);
        m_buffer[m_writeOffset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        m_buffer[m_writeOffset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        m_buffer[m_writeOffset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
        m_writeOffset += 4;
    }

    void writeF32(float value)
    {
        // Преобразуем float в его битовое представление uint32_t (little-endian)
        uint32_t raw;
        std::memcpy(&raw, &value, sizeof(raw));
        writeU32(raw);
    }

    // ────────────────────────────────────────
    // Чтение
    // ────────────────────────────────────────

    uint8_t readU8()
    {
        ensureReadCapacity(1);
        uint8_t value = m_buffer[m_readOffset];
        m_readOffset += 1;
        return value;
    }

    uint16_t readU16()
    {
        ensureReadCapacity(2);
        uint16_t value = static_cast<uint16_t>(m_buffer[m_readOffset])
            | (static_cast<uint16_t>(m_buffer[m_readOffset + 1]) << 8);
        m_readOffset += 2;
        return value;
    }

    uint32_t readU32()
    {
        ensureReadCapacity(4);
        uint32_t value = static_cast<uint32_t>(m_buffer[m_readOffset])
            | (static_cast<uint32_t>(m_buffer[m_readOffset + 1]) << 8)
            | (static_cast<uint32_t>(m_buffer[m_readOffset + 2]) << 16)
            | (static_cast<uint32_t>(m_buffer[m_readOffset + 3]) << 24);
        m_readOffset += 4;
        return value;
    }

    float readF32()
    {
        uint32_t raw = readU32();
        float value;
        std::memcpy(&value, &raw, sizeof(value));
        return value;
    }

    /// Сколько байт записано (только для writer).
    size_t writtenSize() const
    {
        return m_writeOffset;
    }

    /// Сколько байт осталось прочитать (только для reader).
    size_t remainingBytes() const
    {
        return m_buffer.size() - m_readOffset;
    }

private:
    void ensureWriteCapacity(size_t bytes)
    {
        if (m_writeOffset + bytes > m_buffer.size())
            m_buffer.resize(m_writeOffset + bytes);
    }

    void ensureReadCapacity(size_t bytes) const
    {
        if (m_readOffset + bytes > m_buffer.size())
            throw std::runtime_error(
                "Serializer: попытка прочитать " + std::to_string(bytes)
                + " байт, но осталось только " + std::to_string(remainingBytes())
            );
    }

    std::vector<uint8_t>& m_buffer;
    size_t m_writeOffset = 0;
    size_t m_readOffset = 0;
    bool m_isReader = false;
};

} // namespace wingz::net
