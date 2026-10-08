#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int write_all(int fd, const char *data, size_t size) {
    size_t written = 0;

    while (written < size) {
        ssize_t res = write(fd, data + written, size - written);

        if (res > 0) {
            written += (size_t) res;
        } else if (res == 0) {
            errno = EIO;
            return -1;
        } else if (errno != EINTR) {
            return -1;
        }
    }

    return 0;
}

int main(void) {
    char *filename = NULL;
    size_t capacity = 0;
    ssize_t length = getline(&filename, &capacity, stdin);

    if (length == -1) {
        if (ferror(stdin)) perror("getline filename");
        else fprintf(stderr, "Missing output filename\n");
        free(filename);
        return EXIT_FAILURE;
    }

    if (length > 0 && filename[length - 1] == '\n') filename[--length] = '\0';
    if (length > 0 && filename[length - 1] == '\r') filename[--length] = '\0';

    if (length == 0 || memchr(filename, '\0', (size_t) length) != NULL) {
        fprintf(stderr, "Invalid output filename\n");
        free(filename);
        return EXIT_FAILURE;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        free(filename);
        return EXIT_FAILURE;
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        if (close(pipe_fd[0]) == -1) perror("close read end");
        if (close(pipe_fd[1]) == -1) perror("close write end");
        free(filename);
        return EXIT_FAILURE;
    }

    if (child_pid == 0) {
        if (close(pipe_fd[1]) == -1) {
            perror("close write end");
            _exit(EXIT_FAILURE);
        }

        if (pipe_fd[0] != STDIN_FILENO) {
            if (dup2(pipe_fd[0], STDIN_FILENO) == -1) {
                perror("dup2");
                _exit(EXIT_FAILURE);
            }
            if (close(pipe_fd[0]) == -1) {
                perror("close read end");
                _exit(EXIT_FAILURE);
            }
        }

        execl(CHILD_PATH, "child", filename, (char *) NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    }

    free(filename);
    int failed = 0;

    if (close(pipe_fd[0]) == -1) {
        perror("close read end");
        failed = 1;
    }

    if (!failed && signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal SIGPIPE");
        failed = 1;
    }

    fprintf(stderr, "parent PID: %ld, child PID: %ld\n", (long) getpid(), (long) child_pid);

    char *line = NULL;
    size_t line_capacity = 0;

    while (!failed) {
        ssize_t line_length = getline(&line, &line_capacity, stdin);
        if (line_length == -1) {
            if (ferror(stdin)) {
                perror("getline command");
                failed = 1;
            }
            break;
        }

        if (write_all(pipe_fd[1], line, (size_t) line_length) == -1) {
            perror("write pipe");
            failed = 1;
            break;
        }
    }

    free(line);

    if (close(pipe_fd[1]) == -1) {
        perror("close write end");
        failed = 1;
    }

    int status;
    pid_t wait_result;
    do {
        wait_result = waitpid(child_pid, &status, 0);
    } while (wait_result == -1 && errno == EINTR);

    if (wait_result == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        fprintf(stderr, "child exited with status: %d\n", WEXITSTATUS(status));
        if (WEXITSTATUS(status) != EXIT_SUCCESS) failed = 1;
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "child killed by signal: %d\n", WTERMSIG(status));
        failed = 1;
    } else {
        fprintf(stderr, "Unexpected child status\n");
        failed = 1;
    }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
