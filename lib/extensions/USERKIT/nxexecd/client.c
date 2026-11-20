/*
 * user/sys/lib/extensions/services/execsvc_client.c
 * Client stub for the execution service (Phase 9c).
 *
 * Deliberately its OWN translation unit rather than more code inside lib.c:
 * Phase 10a/12 will split service clients out of the libc entirely, and lib.c is
 * already the file whose entanglement those phases exist to undo.  Starting it
 * separate means that split is a Makefile move, not another extraction.
 *
 * WHAT THIS IS FOR: nxexec is meant to be THE standard executor and the POSIX
 * compatibility point for execution.  system() historically bypassed it
 * completely (it spawned the shell itself), so the POSIX surface had its own
 * private execution path — exactly the divergence the daemon exists to remove.
 *
 * WHAT IT IS NOT FOR: interactive foreground jobs.  The service becomes the
 * spawning parent, and while OBJ_CTL_SETOWNER restores AUTHORITY it does NOT
 * move the controlling terminal, so Ctrl-Z/Ctrl-C would break.  That is Phase 9d
 * (ctty handback), and until then interactive jobs keep the in-process path.
 */
#include <execsvc.h>
#include <os1_tty.h>
#include <os1.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

struct execsvc_request {
  int service, request[2], reply[2];
  struct execsvc_spawn_hdr header;
  unsigned used;
  char body[EXECSVC_BODY_MAX];
};

static int execsvc_write_full(int fd, const void *data, unsigned size) {
  if (size > EXECSVC_BODY_MAX) return -EOVERFLOW;
  unsigned done = 0U;
  for (unsigned step = 0U; step < EXECSVC_BODY_MAX && done < size; ++step) {
    long n = write(fd, (const char *)data + done, size - done);
    if (n < 0) return -errno;
    if (n == 0) return -EPIPE;
    if ((unsigned long)n > size - done) return -EIO;
    done += (unsigned)n;
  }
  return done == size ? 0 : -EIO;
}

static int execsvc_read_reply(int fd, struct execsvc_spawn_rep *reply) {
  size_t done = 0U;
  for (size_t step = 0U; step < sizeof(*reply) && done < sizeof(*reply); ++step) {
    long n = read(fd, (char *)reply + done, sizeof(*reply) - done);
    if (n < 0) return -errno;
    if (n == 0) return -EPIPE;
    if ((unsigned long)n > sizeof(*reply) - done) return -EIO;
    done += (size_t)n;
  }
  if (done != sizeof(*reply) || reply->version != EXECSVC_VERSION ||
      reply->pid == 0) return -EINVAL;
  return reply->pid;
}

static int execsvc_close_fd(int *fd) {
  if (*fd < 0) return 0;
  int result = close(*fd);
  if (result != 0) return -errno;
  *fd = -1;
  return 0;
}

static int execsvc_release_request(struct execsvc_request *request) {
  int status = 0;
  for (unsigned i = 0U; i < 2U; ++i) {
    int a = execsvc_close_fd(&request->request[i]);
    int b = execsvc_close_fd(&request->reply[i]);
    if (status == 0 && a != 0) status = a;
    if (status == 0 && b != 0) status = b;
  }
  if (request->service >= 0) {
    long closed = OS1low_handle_close(request->service);
    if (status == 0 && closed != 0) status = (int)closed;
    if (closed == 0) request->service = -1;
  }
  return status;
}

static int execsvc_append_argument(struct execsvc_request *request,
                                    const char *text) {
  if (text == NULL || request->used >= sizeof(request->body)) return -EINVAL;
  size_t size = strnlen(text, EXECSVC_BODY_MAX);
  if (size >= EXECSVC_BODY_MAX ||
      size + 1U > sizeof(request->body) - request->used) return -E2BIG;
  if (memcpy(request->body + request->used, text, size + 1U) !=
      request->body + request->used) return -EIO;
  request->used += (unsigned)(size + 1U);
  return 0;
}

static int execsvc_pack_request(struct execsvc_request *request,
                                 int argc, char *const argv[], unsigned flags) {
  char cwd[EXECSVC_PATH_MAX];
  int status = getcwd(cwd, sizeof(cwd));
  if (status != 0) return status;
  status = execsvc_append_argument(request, cwd);
  if (status != 0) return status;
  for (int i = 0; i < EXECSVC_ARG_MAX && i < argc; ++i) {
    status = execsvc_append_argument(request, argv[i]);
    if (status != 0) return status;
  }
  request->header = (struct execsvc_spawn_hdr){
    .version = EXECSVC_VERSION, .flags = flags,
    .argc = argc, .body_len = request->used
  };
  return 0;
}

static int execsvc_request_channels(struct execsvc_request *request) {
  request->service = OS1_port_open(OS1NX_PORT_EXEC);
  if (request->service < 0) return -errno;
  if (pipe(request->request) != 0 || pipe(request->reply) != 0) return -errno;
  struct ipc_message message = {.type = EXECSVC_REQ_SPAWN};
  int give[2] = {request->request[0], request->reply[1]};
  long sent = OS1_port_send_caps(request->service, &message, give, 2);
  if (sent != (long)sizeof(message)) return sent < 0 ? -errno : -EIO;
  int a = execsvc_close_fd(&request->request[0]);
  int b = execsvc_close_fd(&request->reply[1]);
  return a != 0 ? a : b;
}

static int execsvc_exchange_request(struct execsvc_request *request) {
  int status = execsvc_request_channels(request);
  if (status != 0) return status;
  status = execsvc_write_full(request->request[1], &request->header,
                               sizeof(request->header));
  if (status == 0)
    status = execsvc_write_full(request->request[1], request->body, request->used);
  int closed = execsvc_close_fd(&request->request[1]);
  if (status != 0) return status;
  if (closed != 0) return closed;
  struct execsvc_spawn_rep reply;
  return execsvc_read_reply(request->reply[0], &reply);
}

int execsvc_spawn(int argc, char *const argv[], unsigned int flags) {
  if (argc <= 0 || argc > EXECSVC_ARG_MAX || argv == NULL ||
      (flags & ~SPAWN_FLAGS_ALL) != 0U) return -EINVAL;
  struct execsvc_request request = {
    .service = -1, .request = {-1, -1}, .reply = {-1, -1}
  };
  int result = execsvc_pack_request(&request, argc, argv, flags);
  if (result == 0) result = execsvc_exchange_request(&request);
  int closed = execsvc_release_request(&request);
  /* A confirmed child must never be launched twice through caller fallback
   * merely because teardown of a separate transport descriptor failed. */
  if (closed != 0) OS1_report_error("execsvc.channel_close", closed);
  return result;
}

/* The shared executable resolver is deliberately in the SDK: callers and the
 * execution broker cannot diverge in quoting, HOME, cwd or PATH semantics. */
const char *execsvc_strip_path_quotes(const char *name, char *out, size_t size) {
  if (!name || !out || size == 0U) return NULL;
  size_t n = strlen(name);
  if (n >= 2U && ((name[0] == '"' && name[n - 1U] == '"') ||
                  (name[0] == '\'' && name[n - 1U] == '\''))) {
    n -= 2U;
    if (n >= size) return NULL;
    for (size_t i = 0U; i < EXECSVC_PATH_MAX && i < n; ++i) {
      out[i] = name[i + 1U];
    }
    out[n] = '\0';
    return out;
  }
  return name;
}

/* 0=resolved pathname, 1=bare name requiring /bin then /sys/bin search,
 * negative errno=invalid/truncated input. No implicit truncation. */
int execsvc_resolve_path(const char *name, char *out, size_t size) {
  if (!name || !name[0] || !out || size < 2U) return -EINVAL;
  char clean[EXECSVC_PATH_MAX];
  const char *candidate = execsvc_strip_path_quotes(name, clean, sizeof(clean));
  if (!candidate || !candidate[0]) return -ENAMETOOLONG;
  int n;
  if (candidate[0] == '/') {
    n = snprintf(out, size, "%s", candidate);
    return n < 0 || (size_t)n >= size ? -ENAMETOOLONG : 0;
  }
  if (candidate[0] == '~') {
    const char *home = getenv("HOME");
    if (!home || !home[0]) home = EXECSVC_HOME_DEFAULT;
    if (candidate[1] == '\0')
      n = snprintf(out, size, "%s", home);
    else if (candidate[1] == '/')
      n = snprintf(out, size, "%s%s", home, candidate + 1);
    else
      return -EINVAL;
    return n < 0 || (size_t)n >= size ? -ENAMETOOLONG : 0;
  }
  if (strchr(candidate, '/') != NULL) {
    char cwd[EXECSVC_PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != 0) return -EIO;
    size_t len = strlen(cwd);
    n = snprintf(out, size, len && cwd[len - 1U] == '/'
                                 ? "%s%s" : "%s/%s", cwd, candidate);
    return n < 0 || (size_t)n >= size ? -ENAMETOOLONG : 0;
  }
  return 1;
}

const char *execsvc_basename(const char *path) {
  if (path == NULL) return "";
  const char *base = path;
  for (const char *cursor = path; *cursor != '\0'; ++cursor) {
    if (*cursor == '/') base = cursor + 1;
  }
  return base;
}

int execsvc_window_stable(int pid, int *last_win, int *stable) {
  if (pid <= 0 || last_win == NULL || stable == NULL) return 0;
  int window = window_of_pid(pid);
  if (window > 0 && window == *last_win)
    ++(*stable);
  else
    *stable = window > 0 ? 1 : 0;
  *last_win = window;
  return *stable >= EXECSVC_STABLE_POLLS;
}

static void execsvc_close_redirection_fds(int fds[], int *nfds) {
  if (fds == NULL || nfds == NULL) return;
  for (int i = 0; i < SPAWN_MAX_REDIR && i < *nfds; ++i) {
    if (fds[i] >= 0 && close(fds[i]) != 0)
      OS1_report_error("execsvc.redirection_close", errno);
  }
  *nfds = 0;
}

int execsvc_strip_redirections(int *pargc, char *argv[],
                               struct spawn_redir *redir, int *nredir,
                               int fds[], int *nfds) {
  if (pargc == NULL || argv == NULL || redir == NULL || nredir == NULL ||
      fds == NULL || nfds == NULL || *pargc < 0 ||
      *pargc > EXECSVC_LOCAL_ARG_MAX) return -EINVAL;
  int argc = *pargc;
  int out = 0;
  int status = 0;
  *nredir = 0;
  *nfds = 0;
  for (int i = 0; i < EXECSVC_LOCAL_ARG_MAX && i < argc && status == 0; ++i) {
    char *token = argv[i];
    int child_fd = -1;
    int flags = 0;
    const char *name = NULL;
    if (strcmp(token, "<") == 0) {
      child_fd = 0; flags = O_RDONLY;
    } else if (strcmp(token, ">") == 0) {
      child_fd = 1; flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strcmp(token, ">>") == 0) {
      child_fd = 1; flags = O_WRONLY | O_CREAT | O_APPEND;
    } else if (strcmp(token, "2>") == 0) {
      child_fd = 2; flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (token[0] == '<' && token[1] != '\0') {
      child_fd = 0; flags = O_RDONLY; name = token + 1;
    } else if (token[0] == '2' && token[1] == '>' && token[2] != '\0') {
      child_fd = 2; flags = O_WRONLY | O_CREAT | O_TRUNC; name = token + 2;
    } else if (token[0] == '>' && token[1] == '>' && token[2] != '\0') {
      child_fd = 1; flags = O_WRONLY | O_CREAT | O_APPEND; name = token + 2;
    } else if (token[0] == '>' && token[1] != '\0') {
      child_fd = 1; flags = O_WRONLY | O_CREAT | O_TRUNC; name = token + 1;
    } else {
      argv[out++] = token;
    }
    if (child_fd >= 0) {
      if (name == NULL) {
        if (i + 1 >= argc) status = -EINVAL;
        else name = argv[++i];
      }
      if (status == 0 && *nredir >= SPAWN_MAX_REDIR) status = -E2BIG;
      if (status == 0) {
        int fd = open(name, flags, 0644);
        if (fd < 0) status = -errno;
        else {
          fds[(*nfds)++] = fd;
          redir[*nredir].child_fd = child_fd;
          redir[*nredir].parent_fd = fd;
          redir[*nredir].source_pid = 0;
          ++(*nredir);
        }
      }
    }
  }
  if (status != 0) {
    execsvc_close_redirection_fds(fds, nfds);
    *nredir = 0;
    return status;
  }
  /* The input contract supplies argc slots. A compacted vector has room
   * for a terminator in the removed tail, even at the argument limit. */
  if (out < argc) argv[out] = NULL;
  *pargc = out;
  return 0;
}

int execsvc_launch_explicit(const char *path, int argc, char *const argv[],
                            unsigned int flags, const struct spawn_redir *redirs,
                            int nredir) {
  if (!path || !path[0] || argc < 0 || argc > EXECSVC_LOCAL_ARG_MAX ||
      nredir < 0 || nredir > SPAWN_MAX_REDIR || (nredir && !redirs) ||
      (flags & ~SPAWN_FLAGS_ALL) != 0U) return -EINVAL;
  return (int)OS1low_process_spawn_redir(path, argc, argv, flags, redirs, nredir);
}

int execsvc_resolve_user_program(const char *input, char *resolved,
                                size_t capacity) {
  if (input == NULL || resolved == NULL || capacity < 8U) return -EINVAL;
  char clean[EXECSVC_PATH_MAX];
  const char *name = execsvc_strip_path_quotes(input, clean, sizeof(clean));
  if (name == NULL || name[0] == '\0') return -ENAMETOOLONG;
  int choice = execsvc_resolve_path(name, resolved, capacity);
  if (choice < 0) return choice;
  if (choice != 0) {
    int n = snprintf(resolved, capacity, "/bin/%s", name);
    if (n < 0 || (size_t)n >= capacity) return -ENAMETOOLONG;
  }
  return strncmp(resolved, "/bin/", 5U) == 0 ? 0 : -EPERM;
}

int execsvc_launch_owned(const struct execsvc_owned_launch *launch,
                         char *resolved, size_t capacity) {
  if (launch == NULL || launch->owner_handle < 0 || launch->argc <= 0 ||
      launch->argc > EXECSVC_LOCAL_ARG_MAX || launch->nredir < 0 ||
      launch->nredir > SPAWN_MAX_REDIR ||
      (launch->nredir != 0 && launch->redirections == NULL) ||
      (launch->flags & ~SPAWN_FLAGS_ALL) != 0U) return -EINVAL;
  int status = execsvc_resolve_user_program(launch->argv[0], resolved, capacity);
  if (status != 0) return status;
  struct execsvc_owned_launch copy = *launch;
  copy.argv[0] = resolved;
  struct os1_spawn_request_v1 request = {
      .abi_version = OS1_SPAWN_REQUEST_VERSION, .struct_size = sizeof(request),
      .argc = (uint32_t)copy.argc, .flags = copy.flags,
      .nredir = (uint32_t)copy.nredir,
      .path = (uint64_t)(uintptr_t)resolved,
      .argv = (uint64_t)(uintptr_t)copy.argv,
      .redirections = (uint64_t)(uintptr_t)copy.redirections
  };
  return (int)OS1_object_ctl(copy.owner_handle, OBJ_CTL_SPAWN_OWNED,
                              (long)(uintptr_t)&request);
}

int execsvc_launch_search(int argc, char *argv[], char *resolved,
                          size_t capacity, unsigned int flags,
                          const struct spawn_redir *redirs, int nredir,
                          int user_only) {
  if (argc <= 0 || argc > EXECSVC_LOCAL_ARG_MAX || !argv || !argv[0] ||
      !resolved || capacity < 8U) return -EINVAL;
  if (user_only) {
    int status = execsvc_resolve_user_program(argv[0], resolved, capacity);
    if (status != 0) return status;
    argv[0] = resolved;
    return execsvc_launch_explicit(resolved, argc, argv, flags, redirs, nredir);
  }
  char clean[EXECSVC_PATH_MAX];
  const char *name = execsvc_strip_path_quotes(argv[0], clean, sizeof(clean));
  if (!name || !name[0]) return -ENAMETOOLONG;
  int choice = execsvc_resolve_path(name, resolved, capacity);
  if (choice < 0) return choice;
  if (choice == 0) {
    argv[0] = resolved;
    return execsvc_launch_explicit(resolved, argc, argv, flags, redirs, nredir);
  }
  int n = snprintf(resolved, capacity, "/bin/%s", name);
  if (n < 0 || (size_t)n >= capacity) return -ENAMETOOLONG;
  argv[0] = resolved;
  int pid = execsvc_launch_explicit(resolved, argc, argv, flags, redirs, nredir);
  if (pid > 0) return pid;
  n = snprintf(resolved, capacity, "/sys/bin/%s", name);
  if (n < 0 || (size_t)n >= capacity) return -ENAMETOOLONG;
  argv[0] = resolved;
  return execsvc_launch_explicit(resolved, argc, argv, flags, redirs, nredir);
}


/* System-owned UI commands are spawned directly by a trusted system caller,
 * without replacing its authority with the USER-only hosted-shell policy.
 * This is the same existing spawn-capability contract used at boot, not an
 * ambient grant keyed merely on the executable's pathname. */
int execsvc_launch_system_gui(const char *path) {
  if (path == NULL || strncmp(path, "/sys/bin/", 9U) != 0 ||
      path[9] == '\0') return -EINVAL;
  int level = PLVL_GUEST;
  unsigned int caps = 0U;
  int result = OS1_identity(&level, &caps);
  if (result != 0) return result;
  if (level > PLVL_ROOT || (caps & CAP_SPAWN) == 0U) return -EPERM;
  return (int)spawn_level(path, PLVL_ROOT);
}

/* TTY hosting is a composition above the base execution model. nxexec and
 * nxttyd own terminal hosting plus GUI/CLI classification; shell job control
 * remains a separate nxjobs extension over foreground process capabilities. */
int execsvc_launch_hosted(int argc, char *const argv[]) {
  if (argc < 1 || argc >= EXECSVC_LOCAL_ARG_MAX || !argv || !argv[0])
    return -EINVAL;
  char *full[EXECSVC_LOCAL_ARG_MAX];
  full[0] = (char *)"/sys/bin/nxexec";
  for (int i = 0; i < EXECSVC_LOCAL_ARG_MAX - 1 && i < argc; ++i) {
    if (!argv[i]) return -EINVAL;
    full[i + 1] = argv[i];
  }
  return execsvc_launch_explicit("/sys/bin/nxexec", argc + 1, full,
                                SPAWN_FLAG_DETACHED, NULL, 0);
}

/* A directly hosted interactive program already has a session. A descendant
 * inherits its output port for commands, but opening another interactive
 * terminal starts a fresh host. This test selects presentation only; FAMILY
 * port acquisition still enforces the actual writer authority. */
int execsvc_open_terminal(const char *program) {
  char port[64], parent_port[64];
  if (!program || !program[0]) return -EINVAL;
  int parent = getppid();
  if (parent < 0) return -errno;
  int size = snprintf(parent_port, sizeof(parent_port), "%s%d",
                      OS1_TTY_PORT_PREFIX, parent);
  if (size <= 0 || (size_t)size >= sizeof(parent_port)) return -EOVERFLOW;
  if (OS1_env_get(OS1_TTY_ENV, port, sizeof(port)) == 0 &&
      strcmp(port, parent_port) == 0) return 0;
  char *job[] = {(char *)program};
  return execsvc_launch_hosted(1, job);
}

int execsvc_launch(int argc, char *argv[], enum execsvc_launch_mode mode,
                   const struct spawn_redir *redirs, int nredir,
                   char *resolved, size_t capacity) {
  if (mode != EXECSVC_MODE_FOREGROUND && mode != EXECSVC_MODE_DETACHED)
    return -EINVAL;
  return execsvc_launch_search(argc, argv, resolved, capacity,
      mode == EXECSVC_MODE_DETACHED ? SPAWN_FLAG_DETACHED : 0U,
      redirs, nredir, 0);
}
