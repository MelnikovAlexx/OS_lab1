#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


static const char CHILD1_NAME[] = "child1";
static const char CHILD2_NAME[] = "child2";
static const size_t MAX_LEN = 4096;

void error(const char *msg) {
	write(STDERR_FILENO, msg, strlen(msg));
	exit(EXIT_FAILURE); 
}

void get_progpath(char *buf, size_t bufsize) {
	ssize_t len = readlink("/proc/self/exe", buf, bufsize - 1);
	if (len == -1) 
        error("error: failed to read full program path\n");

    buf[len] = '\0';
	while (len > 0 && buf[len] != '/')
		--len;
	buf[len] = '\0';
}


ssize_t read_line(int fd, char **line, size_t *cap) {
    if (*line == NULL) {
        *cap = MAX_LEN;
        *line = malloc(*cap);
        if (*line == NULL) 
			return -1;
    }
    size_t len = 0;
	while (true) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n == -1) 
			return -1;
        if (n == 0) {      
            if (len == 0) 
				return 0;
            break;
        }
        if (c == '\n') 
			break;
        (*line)[len++] = c;
    }
    (*line)[len] = '\0';
    return (ssize_t)len;
}


int main() {
	char progdir[MAX_LEN];
	get_progpath(progdir, sizeof(progdir) - 1);

	char child1_path[MAX_LEN], child2_path[MAX_LEN];
    snprintf(child1_path, sizeof(child1_path), "%s/%s", progdir, CHILD1_NAME);
    snprintf(child2_path, sizeof(child2_path), "%s/%s", progdir, CHILD2_NAME);

	char *filename1 = NULL, *filename2 = NULL;
    size_t cap1 = 0, cap2 = 0;

    const char hint1[] = "filename1: ";
    write(STDERR_FILENO, hint1, sizeof(hint1) - 1);
    if (read_line(STDIN_FILENO, &filename1, &cap1) <= 0) 
        error("error: no filename1\n");

    const char hint2[] = "filename2: ";
    write(STDERR_FILENO, hint2, sizeof(hint2) - 1);
    if (read_line(STDIN_FILENO, &filename2, &cap2) <= 0) 
        error("error: no filename2\n");

	int file1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (file1 == -1) 
		error("error: failed to open file1\n");
    int file2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (file2 == -1) 
		error("error: failed to open file2\n");


	int p1[2], p2[2];
    if (pipe(p1) == -1) 
		error("error: failed to create pipe\n");
    if (pipe(p2) == -1) 
		error("error: failed to create pipe\n");


    const pid_t c1 = fork();
    if (c1 == -1) 
		error("error: failed to spawn new process\n");
    if (c1 == 0) {
		{
			pid_t pid = getpid(); 

			char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg),
				"%d: I'm a child\n", pid);
			write(STDOUT_FILENO, msg, length);
		}
        close(p1[1]);         
        close(p2[0]); close(p2[1]);  
        close(file2);  

        if (dup2(p1[0], STDIN_FILENO) == -1) 
			error("error: failed dup2 stdin\n");
        if (dup2(file1, STDOUT_FILENO) == -1) 
			error("error: failed dup2 stdout\n");

        close(p1[0]);
        close(file1);

        char *const args[] = {CHILD1_NAME, NULL};
		int32_t status = execv(child1_path, args);
        if (status == -1) 
			error("error: failed to exec into new exectuable image\n");
    }

	pid_t c2 = fork();
    if (c2 == -1) 
		error("error: failed to spawn new process\n");
    if (c2 == 0) {
        {
			pid_t pid = getpid();

			char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg),
				"%d: I'm a child\n", pid);
			write(STDOUT_FILENO, msg, length);
		}
        close(p2[1]);         
        close(p1[0]); close(p1[1]);  
        close(file1);  

        if (dup2(p2[0], STDIN_FILENO) == -1) 
			error("error: failed dup2 stdin\n");
        if (dup2(file2, STDOUT_FILENO) == -1) 
			error("error: failed dup2 stdout\n");

        close(p2[0]);
        close(file2);

        char *const args[] = {CHILD2_NAME, NULL};
		int32_t status = execv(child2_path, args);
        if (status == -1) 
			error("error: failed to exec into new exectuable image\n");
    }

	close(p1[0]); 
	close(p2[0]); 
    close(file1); 
	close(file2); 


	char *line = NULL;
    size_t cap = 0;
    uint32_t num_of_line = 0;
    while (true) {
        ssize_t len = read_line(STDIN_FILENO, &line, &cap);
        if (len == -1) 
            error("read stdin"); 
        if (len == 0)
            break;                

        ++num_of_line;
        int dst = (num_of_line % 2) ? p1[1] : p2[1];

        if (write(dst, line, (size_t)len) == -1) 
            error("write line"); 
        if (write(dst, "\n", 1) == -1) 
            error("write new line"); 
    }

	close(p1[1]);
    close(p2[1]);

    wait(NULL);
    wait(NULL);

    free(line);
    free(filename1);
    free(filename2);
    return 0;
}