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

class Error;

class NetstringInput {
    enum class State {
        HEADER,
        VALUE,
        FINISHED,
    };

    State state;

    char header_buffer[32];
    size_t header_position;

    uint8_t *value_buffer;
    size_t value_size, value_position;

    const size_t max_size;

public:
    NetstringInput(size_t _max_size)
        :state(State::HEADER),
         header_position(0), value_buffer(nullptr),
         max_size(_max_size) {}

    ~NetstringInput();

    enum class Result {
        MORE,
        ERROR,
        CLOSED,
        FINISHED,
    };

    Result Receive(int fd, Error &error);

    void *GetValue() const {
        assert(state == State::FINISHED);

        return value_buffer;
    }

    size_t GetSize() const {
        assert(state == State::FINISHED);

        return value_size;
    }

private:
    Result ReceiveHeader(int fd, Error &error);
    Result ValueData(size_t nbytes, Error &error);
    Result ReceiveValue(int fd, Error &error);
};

#endif
