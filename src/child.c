#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

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

static int parse_sum(const char *line, int *sum_out) {
    const char *current = line;
    int sum = 0;
    int found = 0;

    while (*current != '\0') {
        while (isspace((unsigned char)*current)) current++;
        if (*current == '\0') break;

        errno = 0;
        char *end;
        long value = strtol(current, &end, 10);

        if (current == end || errno == ERANGE || value < INT_MIN || value > INT_MAX) return -1;
        if (*end != '\0' && !isspace((unsigned char)*end)) return -1;

        if ((value > 0 && sum > INT_MAX - value) || (value < 0 && sum < INT_MIN - value)) return -2;

        sum += (int) value;
        found = 1;
        current = end;
    }

    if (!found) return -1;
    *sum_out = sum;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: child <output-file>\n");
        return EXIT_FAILURE;
    }

    int output_fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644); //rw-r--r--
    if (output_fd == -1) {
        perror("open output file");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "child PID: %ld, parent PID: %ld\n", (long) getpid(), (long) getppid());

    char *line = NULL;
    size_t capacity = 0;
    size_t line_number = 0;
    int failed = 0;

    while (1) {
        ssize_t length = getline(&line, &capacity, stdin);
        if (length == -1) {
            if (ferror(stdin)) {
                perror("getline stdin");
                failed = 1;
            }
            break;
        }

        line_number++;
        // line is invalid if '\0' present in mid of it
        if (memchr(line, '\0', (size_t) length) != NULL) {
            fprintf(stderr, "Invalid command on line %zu: NUL byte\n", line_number);
            failed = 1;
            break;
        }

        int sum;
        int parse_result = parse_sum(line, &sum);
        if (parse_result != 0) {
            if (parse_result == -2) {
                fprintf(stderr, "Integer sum overflow on line %zu\n", line_number);
            } else {
                fprintf(stderr, "Invalid command on line %zu\n", line_number);
            }
            failed = 1;
            break;
        }

        char output[32];
        int output_length = snprintf(output, sizeof(output), "%d\n", sum);
        if (output_length < 0 || (size_t) output_length >= sizeof(output)) {
            fprintf(stderr, "Failed to format sum\n");
            failed = 1;
            break;
        }

        if (write_all(output_fd, output, (size_t) output_length) == -1) {
            perror("write output file");
            failed = 1;
            break;
        }
    }

    free(line);

    if (close(output_fd) == -1) {
        perror("close output file");
        failed = 1;
    }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
