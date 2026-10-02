#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static const size_t MAX_LEN = 4096;

static int is_vowel(unsigned char c) {
    switch (tolower(c)) {
    case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':   
        return 1;
    default:
        return 0;
    }
}

int main() {
    char line[MAX_LEN];
    char out[MAX_LEN];
    while (true) {
        ssize_t n = read(STDIN_FILENO, line, sizeof(line));
        if (n == -1) { 
            const char msg[] = "error: reading failed\n";
		    write(STDERR_FILENO, msg, sizeof(msg));
		    exit(EXIT_FAILURE);
        }
        if (n == 0) 
            break;

        size_t line_len = 0;
        for (ssize_t i = 0; i < n; ++i) {
            unsigned char c = (unsigned char)line[i];
            if (!is_vowel(c)) 
                out[line_len++] = (char)c;
        }

        if (line_len > 0)
            if (write(STDOUT_FILENO, out, line_len) == -1) { 
                const char msg[] = "error: writitng failed\n";
		        write(STDERR_FILENO, msg, sizeof(msg));
		        exit(EXIT_FAILURE); 
            }
    }
    return 0;
}