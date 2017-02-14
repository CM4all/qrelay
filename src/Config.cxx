/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "Config.hxx"
#include "net/Resolver.hxx"
#include "net/AddressInfo.hxx"
#include "util/Tokenizer.hxx"
#include "util/RuntimeError.hxx"
#include "io/TextFile.hxx"
#include "Mail.hxx"

#include <stdexcept>

#include <string.h>

void
Action::ParseConnect(Tokenizer &tokenizer)
{
    assert(!IsDefined());

    const char *value = tokenizer.NextString();
    if (value == nullptr || *value == 0)
        throw std::runtime_error("missing value");

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    const auto ai = Resolve(value, 628, &hints);
    connect = ai.front();
    type = Type::CONNECT;
}

void
Action::Parse(Tokenizer &tokenizer)
{
    assert(!IsDefined());

    const char *a = tokenizer.NextWord();
    if (a == nullptr || *a == 0)
        throw std::runtime_error("missing action");

    if (strcmp(a, "connect") == 0) {
        ParseConnect(tokenizer);
    } else if (strcmp(a, "exec") == 0) {
        const char *p = tokenizer.NextString();
        if (p == nullptr)
            throw std::runtime_error("missing program");

        type = Type::EXEC;

        unsigned n = 0;

        do {
            if (++n > MAX_EXEC)
                throw std::runtime_error("too many arguments");

            exec.emplace_back(p);
            p = tokenizer.NextString();
        } while (p != nullptr);
    } else if (strcmp(a, "discard") == 0) {
        type = Type::DISCARD;
    } else if (strcmp(a, "reject") == 0) {
        type = Type::REJECT;
    } else {
        throw FormatRuntimeError("unknown action: %s", a);
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

void
Config::Condition::Parse(Tokenizer &tokenizer)
{
    assert(!IsDefined());

    const char *op = tokenizer.NextWord();
    if (op == nullptr || *op == 0)
        throw std::runtime_error("missing operator");

    if (strcmp(op, "exclusive_recipient") != 0)
        throw FormatRuntimeError("unknown operator: %s", op);

    const char *value = tokenizer.NextString();
    if (value == nullptr || *value == 0)
        throw std::runtime_error("missing value");

    recipient = value;
}

void
Config::Rule::Parse(Tokenizer &tokenizer)
{
    condition.Parse(tokenizer);
    action.Parse(tokenizer);
}

const Action *
Config::GetAction(const QmqpMail &mail) const
{
    for (const auto &rule : rules) {
        if (rule.condition.Match(mail))
            return &rule.action;
    }

    return &action;
}

void
Config::ParseLine(Tokenizer &tokenizer)
{
    const char *key = tokenizer.NextWord();
    assert(key != nullptr);

    if (strcmp(key, "listen") == 0) {
        const char *value = tokenizer.NextString();
        if (value == nullptr || *value == 0)
            throw std::runtime_error("missing value");

        if (!listen.IsNull())
            throw FormatRuntimeError("duplicate '%s'", key);

        listen.SetLocal(value);
    } else if (strcmp(key, "default") == 0) {
        if (action.IsDefined())
            throw FormatRuntimeError("duplicate '%s'", key);

        action.Parse(tokenizer);
    } else if (strcmp(key, "connect") == 0) {
        if (action.IsDefined())
            throw FormatRuntimeError("duplicate '%s'", key);

        action.ParseConnect(tokenizer);
    } else if (strcmp(key, "rule") == 0) {
        Rule rule;
        rule.Parse(tokenizer);
        rules.push_back(rule);
    } else {
        throw FormatRuntimeError("unknown option '%s'", key);
    }
}

void
Config::LoadFile(TextFile &file)
{
    char *line;
    while ((line = file.ReadLine()) != nullptr) {
        try {
            Tokenizer tokenizer(line);
            if (tokenizer.IsEnd() || tokenizer.CurrentChar() == '#')
                continue;

            ParseLine(tokenizer);

            if (!tokenizer.IsEnd())
                throw std::runtime_error("too many arguments");
        } catch (const std::runtime_error &) {
            std::throw_with_nested(FormatRuntimeError("%s line %u",
                                                      file.GetPath(),
                                                      file.GetLineNumber()));
        }
    }

    if (listen.IsNull())
        throw FormatRuntimeError("no 'listen' in %s", file.GetPath());

    if (!action.connect.IsDefined())
        throw FormatRuntimeError("no 'connect' in %s", file.GetPath());
}

void
Config::LoadFile(const char *path)
{
    TextFile file(path);
    LoadFile(file);
}
