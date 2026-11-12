#include <nxexample_echo.h>
#include <os1.h>
#include <string.h>
#include <unistd.h>

int NX_example_echo(const char *message, char *reply, size_t reply_size) {
  struct ipc_message request = {0};
  int pipefd[2] = {-1, -1};
  if (message == NULL || reply == NULL || reply_size < 2U)
    return -EINVAL;
  int service = OS1_port_open(NXEXAMPLE_ECHO_PORT);
  if (service < 0)
    return service;
  if (pipe(pipefd) != 0) {
    OS1low_handle_close(service);
    return -EIO;
  }
  request.type = IPC_TYPE_RAW;
  const size_t offset = sizeof(int);
  size_t length = strnlen(message, sizeof(request.payload) - offset - 1U);
  memcpy(request.payload + offset, message, length);
  request.payload[offset + length] = '\0';
  long sent = OS1_port_send_caps(service, &request, &pipefd[1], 1);
  close(pipefd[1]);
  OS1low_handle_close(service);
  if (sent != (long)sizeof(request)) {
    close(pipefd[0]);
    return sent < 0 ? (int)sent : -EIO;
  }
  long got = read(pipefd[0], reply, reply_size - 1U);
  close(pipefd[0]);
  if (got < 0)
    return (int)got;
  reply[(size_t)got] = '\0';
  return 0;
}
