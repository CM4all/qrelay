/*
 * A netstring input buffer.
 *
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef NETSTRING_INPUT_HXX
#define NETSTRING_INPUT_HXX

#include <cstddef>
#include <cassert>
#include <cstdint>

class NetstringInput {
    enum class State {
        HEADER,
        VALUE,
        FINISHED,
    };

    State state = State::HEADER;

    char header_buffer[32];
    size_t header_position = 0;

    uint8_t *value_buffer = nullptr;
    size_t value_size, value_position;

    const size_t max_size;

public:
    explicit NetstringInput(size_t _max_size)
        :max_size(_max_size) {}

    ~NetstringInput();

    enum class Result {
        MORE,
        CLOSED,
        FINISHED,
    };

    /**
     * Throws std::runtime_error on error.
     */
    Result Receive(int fd);

    void *GetValue() const {
        assert(state == State::FINISHED);

        return value_buffer;
    }

    size_t GetSize() const {
        assert(state == State::FINISHED);

        return value_size;
    }

private:
    Result ReceiveHeader(int fd);
    Result ValueData(size_t nbytes);
    Result ReceiveValue(int fd);
};

#endif
