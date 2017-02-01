/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Config.hxx"
#include "net/Resolver.hxx"
#include "net/AddressInfo.hxx"
#include "util/Tokenizer.hxx"
#include "util/Error.hxx"
#include "util/Domain.hxx"
#include "io/TextFile.hxx"
#include "Mail.hxx"

#include <stdexcept>

#include <string.h>

static constexpr Domain config_domain("config");

bool
Config::Action::ParseConnect(Tokenizer &tokenizer, Error &error)
{
    assert(!IsDefined());

    const char *value = tokenizer.NextString(error);
    if (value == nullptr || *value == 0) {
        if (!error.IsDefined())
            error.Set(config_domain, "missing value");
        return false;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    const auto ai = Resolve(value, 628, &hints);
    connect = ai.front();
    type = Type::CONNECT;
    return true;
}

bool
Config::Action::Parse(Tokenizer &tokenizer, Error &error)
{
    assert(!IsDefined());

    const char *a = tokenizer.NextWord(error);
    if (a == nullptr || *a == 0) {
        if (!error.IsDefined())
            error.Set(config_domain, "missing action");
        return false;
    }

    if (strcmp(a, "connect") == 0) {
        return ParseConnect(tokenizer, error);
    } else if (strcmp(a, "exec") == 0) {
        const char *p = tokenizer.NextString(error);
        if (p == nullptr) {
            if (!error.IsDefined())
                error.Set(config_domain, "missing program");
            return false;
        }

        type = Type::EXEC;

        unsigned n = 0;

        do {
            if (++n > MAX_EXEC) {
                error.Set(config_domain, "too many arguments");
                return false;
            }

            exec.emplace_back(p);
            p = tokenizer.NextString(error);
        } while (p != nullptr);

        return !error.IsDefined();
    } else if (strcmp(a, "discard") == 0) {
        type = Type::DISCARD;
        return true;
    } else if (strcmp(a, "reject") == 0) {
        type = Type::REJECT;
        return true;
    } else {
        error.Format(config_domain, "unknown action: %s", a);
        return false;
    }

}

bool
Config::Condition::Match(const QmqpMail &mail) const
{
    bool match_any = false, match_all = true;

    for (const auto &i : mail.recipients) {
        const bool equals = i.size == recipient.length() &&
            memcmp(i.data, recipient.data(), i.size) == 0;
        if (equals)
            match_any = true;
        else
            match_all = false;
    }

    if (!match_any)
        return false;

    if (!match_all)
        throw std::runtime_error("partial recipient match");

    return true;
}

bool
Config::Condition::Parse(Tokenizer &tokenizer, Error &error)
{
    assert(!IsDefined());

    const char *op = tokenizer.NextWord(error);
    if (op == nullptr || *op == 0) {
        if (!error.IsDefined())
            error.Set(config_domain, "missing operator");
        return false;
    }

    if (strcmp(op, "exclusive_recipient") != 0) {
        error.Format(config_domain, "unknown operator: %s", op);
        return false;
    }

    const char *value = tokenizer.NextString(error);
    if (value == nullptr || *value == 0) {
        if (!error.IsDefined())
            error.Set(config_domain, "missing value");
        return false;
    }

    recipient = value;
    return true;
}

bool
Config::Rule::Parse(Tokenizer &tokenizer, Error &error)
{
    return condition.Parse(tokenizer, error) &&
        action.Parse(tokenizer, error);
}

const Config::Action *
Config::GetAction(const QmqpMail &mail) const
{
    for (const auto &rule : rules) {
        if (rule.condition.Match(mail))
            return &rule.action;
    }

    return &action;
}

bool
Config::ParseLine(Tokenizer &tokenizer, Error &error)
{
    const char *key = tokenizer.NextWord(error);
    if (key == nullptr) {
        assert(error.IsDefined());
        return false;
    }

    if (strcmp(key, "listen") == 0) {
        const char *value = tokenizer.NextString(error);
        if (value == nullptr || *value == 0) {
            if (!error.IsDefined())
                error.Set(config_domain, "missing value");
            return false;
        }

        if (!listen.empty()) {
            error.Format(config_domain, "duplicate '%s'", key);
            return false;
        }

        listen = value;
        return true;
    } else if (strcmp(key, "default") == 0) {
        if (action.IsDefined()) {
            error.Format(config_domain, "duplicate '%s'", key);
            return false;
        }

        return action.Parse(tokenizer, error);
    } else if (strcmp(key, "connect") == 0) {
        if (action.IsDefined()) {
            error.Format(config_domain, "duplicate '%s'", key);
            return false;
        }

        return action.ParseConnect(tokenizer, error);
    } else if (strcmp(key, "rule") == 0) {
        Rule rule;
        if (!rule.Parse(tokenizer, error))
            return false;

        rules.push_back(rule);
        return true;
    } else {
        error.Format(config_domain, "unknown option '%s'", key);
        return false;
    }
}

bool
Config::LoadFile(TextFile &file, Error &error)
{
    char *line;
    while ((line = file.ReadLine()) != nullptr) {
        Tokenizer tokenizer(line);
        if (tokenizer.IsEnd() || tokenizer.CurrentChar() == '#')
            continue;

        if (!ParseLine(tokenizer, error)) {
            file.PrefixError(error);
            return false;
        }

        if (!tokenizer.IsEnd()) {
            error.Set(config_domain, "too many arguments");
            file.PrefixError(error);
            return false;
        }
    }

    if (listen.empty()) {
        error.Format(config_domain, "no 'listen' in %s", file.GetPath());
        return false;
    }

    if (!action.connect.IsDefined()) {
        error.Format(config_domain, "no 'connect' in %s", file.GetPath());
        return false;
    }

    return true;
}

bool
Config::LoadFile(const char *path, Error &error)
{
    TextFile file(path);
    return LoadFile(file, error);
}
