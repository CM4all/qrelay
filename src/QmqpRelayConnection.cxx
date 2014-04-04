/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "QmqpRelayConnection.hxx"
#include "Mail.hxx"
#include "Config.hxx"
#include "util/ConstBuffer.hxx"
#include "util/Error.hxx"

#include <unistd.h>
#include <fcntl.h>

void
QmqpRelayConnection::OnRequest(void *data, size_t size)
{
    QmqpMail mail;
    if (!mail.Parse(ConstBuffer<char>::FromVoid({data, size}))) {
        if (SendResponse("Dmalformed input"))
            delete this;
        return;
    }

    connect.OnConnect(std::bind(&QmqpRelayConnection::OnConnect, this,
                                std::placeholders::_1, std::placeholders::_1,
                                data, size));
    connect.OnError([this](Error &&error){
            logger(error.GetMessage());
            if (SendResponse("Zconnect failed"))
                delete this;
        });

    Error error;
    const Config::Action *action = config.GetAction(mail, error);
    if (action == nullptr) {
        logger(error.GetMessage());
        if (SendResponse("Drule error"))
            delete this;
        return;
    }

    switch (action->type) {
    case Config::Action::Type::UNDEFINED:
        assert(false);
        gcc_unreachable();

    case Config::Action::Type::DISCARD:
        if (SendResponse("Kdiscarded"))
            delete this;
        break;

    case Config::Action::Type::REJECT:
        if (SendResponse("Drejected"))
            delete this;
        break;

    case Config::Action::Type::CONNECT:
        connect.Connect(action->connect);
        break;

    case Config::Action::Type::EXEC:
        Exec(*action, data, size);
        break;
    }
}

void
QmqpRelayConnection::Exec(const Config::Action &action,
                          void *data, size_t size)
{
    assert(action.type == Config::Action::Type::EXEC);
    assert(!action.exec.empty());

    int stdin_pipe[2], stdout_pipe[2];

    if (pipe2(stdin_pipe, O_CLOEXEC|O_NONBLOCK) < 0) {
        Error error;
        error.SetErrno("pipe() failed");
        OnError(std::move(error));
        return;
    }

    if (pipe2(stdout_pipe, O_CLOEXEC|O_NONBLOCK) < 0) {
        Error error;
        error.SetErrno("pipe() failed");
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        OnError(std::move(error));
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        Error error;
        error.SetErrno("fork() failed");
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        OnError(std::move(error));
        return;
    }

    if (pid == 0) {
        dup2(stdin_pipe[0], 0);
        dup2(stdout_pipe[1], 1);

        /* disable O_NONBLOCK */
        fcntl(0, F_SETFL, fcntl(0, F_GETFL) & ~O_NONBLOCK);
        fcntl(1, F_SETFL, fcntl(1, F_GETFL) & ~O_NONBLOCK);

        char *argv[Config::Action::MAX_EXEC + 1];

        unsigned n = 0;
        for (const auto &i : action.exec)
            argv[n++] = const_cast<char *>(i.c_str());

        argv[n] = nullptr;

        execv(argv[0], argv);
        _exit(1);
    }

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    OnConnect(stdin_pipe[1], stdout_pipe[0], data, size);
}

void
QmqpRelayConnection::OnConnect(int out_fd, int in_fd,
                               const void *data, size_t size)
{
    client.OnResponse(std::bind(&QmqpRelayConnection::OnResponse, this,
                                std::placeholders::_1,
                                std::placeholders::_2));
    client.OnError([this](Error &&error){
            if (SendResponse("Zrelay failed"))
                delete this;
        });

    client.Request(out_fd, in_fd, data, size);
}

void
QmqpRelayConnection::OnResponse(const void *data, size_t size)
{
    if (SendResponse(data, size))
        delete this;
}

void
QmqpRelayConnection::OnError(Error &&error)
{
    logger(error.GetMessage());
    delete this;
}

void
QmqpRelayConnection::OnDisconnect()
{
    delete this;
}
