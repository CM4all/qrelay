/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef SNOWBALL_LOGGER_HXX
#define SNOWBALL_LOGGER_HXX

#include <string>
#include <iostream>

class Logger {
    std::string prefix;

public:
    Logger() = default;

    template<typename N>
    Logger(const Logger &parent, N &&name):prefix(parent.prefix) {
        if (!parent.prefix.empty()) {
            prefix.assign(parent.prefix.begin(),
                          parent.prefix.end() - 2);
            prefix += '/';
        }

        prefix += std::forward<N>(name);
        prefix += ": ";
    }

    template<typename... Params>
    void operator()(Params... params) {
        DoLog(std::cout, prefix, params...) << std::endl;
    }

private:
    std::ostream &DoLog(std::ostream &o) {
        return o;
    }

    template<typename T, typename... Params>
    std::ostream &DoLog(std::ostream &o, const T &t, Params... params) {
        return DoLog(o << t, params...);
    }
};

#endif
