/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef OSTREAM_EXCEPTION_HXX
#define OSTREAM_EXCEPTION_HXX

#include <stdexcept>

namespace std {

template<class _Traits>
inline basic_ostream<char, _Traits> &
operator<<(basic_ostream<char, _Traits> &out, const exception &e)
{
    out << e.what();

    try {
        std::rethrow_if_nested(e);
        return out;
    } catch (const std::exception &nested) {
        return out << "; " << nested;
    } catch (...) {
        return out << "; Unrecognized nested exception";
    }
}

template<class _Traits>
inline basic_ostream<char, _Traits> &
operator<<(basic_ostream<char, _Traits> &out, exception_ptr ep)
{
    try {
        std::rethrow_exception(ep);
    } catch (const std::exception &e) {
        return out << e;
    } catch (...) {
        return out << "Unrecognized exception";
    }
}

}

#endif
