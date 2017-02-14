/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#include "QmqpRelayConnection.hxx"
#include "Mail.hxx"
#include "Config.hxx"
#include "system/Error.hxx"
#include "util/OstreamException.hxx"

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

void
QmqpRelayConnection::OnRequest(void *data, size_t size)
{
    assert(request.empty());

    QmqpMail mail;
    if (!mail.Parse(ConstBuffer<char>::FromVoid({data, size}))) {
        if (SendResponse("Dmalformed input"))
            delete this;
        return;
    }

    tail = mail.tail.ToVoid();
    request.push_back(mail.message.ToVoid());

    const Action *action;
    try {
        action = config.GetAction(mail);
    } catch (const std::runtime_error &e) {
        logger(e);
        if (SendResponse("Drule error"))
            delete this;
        return;
    }

    switch (action->type) {
    case Action::Type::UNDEFINED:
        assert(false);
        gcc_unreachable();

    case Action::Type::DISCARD:
        if (SendResponse("Kdiscarded"))
            delete this;
        break;

    case Action::Type::REJECT:
        if (SendResponse("Drejected"))
            delete this;
        break;

    case Action::Type::CONNECT:
        connect.Connect(action->connect);
        break;

    case Action::Type::EXEC:
        Exec(*action);
        break;
    }
}

void
QmqpRelayConnection::Exec(const Action &action)
{
    assert(action.type == Action::Type::EXEC);
    assert(!action.exec.empty());

    int stdin_pipe[2], stdout_pipe[2];

    if (pipe2(stdin_pipe, O_CLOEXEC|O_NONBLOCK) < 0) {
        OnError(std::make_exception_ptr(MakeErrno("pipe() failed")));
        return;
    }

    if (pipe2(stdout_pipe, O_CLOEXEC|O_NONBLOCK) < 0) {
        const int e = errno;
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        OnError(std::make_exception_ptr(MakeErrno(e, "pipe() failed")));
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        const int e = errno;
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        OnError(std::make_exception_ptr(MakeErrno(e, "fork() failed")));
        return;
    }

    if (pid == 0) {
        dup2(stdin_pipe[0], 0);
        dup2(stdout_pipe[1], 1);

        /* disable O_NONBLOCK */
        fcntl(0, F_SETFL, fcntl(0, F_GETFL) & ~O_NONBLOCK);
        fcntl(1, F_SETFL, fcntl(1, F_GETFL) & ~O_NONBLOCK);

        char *argv[Action::MAX_EXEC + 1];

        unsigned n = 0;
        for (const auto &i : action.exec)
            argv[n++] = const_cast<char *>(i.c_str());

        argv[n] = nullptr;

        execv(argv[0], argv);
        _exit(1);
    }

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    OnConnect(stdin_pipe[1], stdout_pipe[0]);
}

void
QmqpRelayConnection::OnConnect(int out_fd, int in_fd)
{
    client.OnResponse(std::bind(&QmqpRelayConnection::OnResponse, this,
                                std::placeholders::_1,
                                std::placeholders::_2));
    client.OnError([this](std::exception_ptr){
            if (SendResponse("Zrelay failed"))
                delete this;
        });

    struct ucred cred;
    socklen_t len = sizeof (cred);
    if (getsockopt(GetFD(), SOL_SOCKET, SO_PEERCRED, &cred, &len) == 0) {
        int length = sprintf(received_buffer,
                             "Received: from PID=%u UID=%u with QMQP\r\n",
                             unsigned(cred.pid), unsigned(cred.uid));
        request.emplace_front(received_buffer, length);
    }

    generator(request, false);
    request.push_back(tail);

    client.Request(out_fd, in_fd, std::move(request));
}

void
QmqpRelayConnection::OnResponse(const void *data, size_t size)
{
    if (SendResponse(data, size))
        delete this;
}

void
QmqpRelayConnection::OnError(std::exception_ptr ep)
{
    logger(ep);
    delete this;
}

void
QmqpRelayConnection::OnDisconnect()
{
    delete this;
}

void
QmqpRelayConnection::OnSocketConnectSuccess(SocketDescriptor &&_fd)
{
    const int connection_fd = _fd.Steal();
    OnConnect(connection_fd, connection_fd);
}

void
QmqpRelayConnection::OnSocketConnectError(std::exception_ptr ep)
{
    logger(ep);
    if (SendResponse("Zconnect failed"))
        delete this;
}
