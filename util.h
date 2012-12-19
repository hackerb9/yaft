/* See LICENSE for licence details. */
void error(char *str)
{
	/* for DEBUG
	void *buffer[BUFSIZE];
	int size;

	size = backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
	backtrace_symbols_fd(buffer, size, STDERR_FILENO);
	*/
	perror(str);
	exit(EXIT_FAILURE);
}

void fatal(char *str)
{
	fprintf(stderr, "%s\n", str);
	exit(EXIT_FAILURE);
}

int eopen(const char *path, int flag)
{
	int fd;

	if ((fd = open(path, flag)) < 0) {
		fprintf(stderr, "cannot open \"%s\"\n", path);
		error("open");
	}
	return fd;
}

void eclose(int fd)
{
	if (close(fd) < 0)
		error("close");
}

FILE *efopen(char *path, char *mode)
{
	FILE *fp;

	if ((fp = fopen(path, mode)) == NULL) {
		fprintf(stderr, "cannot open \"%s\"\n", path);
		error("fopen");
	}
	return fp;
}

void efclose(FILE *fp)
{
	if (fclose(fp) < 0)
		error("fclose");
}

void *emmap(int addr, size_t len, int prot, int flag, int fd, off_t offset)
{
	uint32_t *fp;

	if ((fp = (uint32_t *) mmap(0, len, prot, flag, fd, offset)) == MAP_FAILED)
		error("mmap");
	return fp;
}

void emunmap(void *ptr, size_t len)
{
	if (munmap(ptr, len) < 0)
		error("munmap");
}

void *emalloc(size_t size)
{
	void *p;

	if ((p = calloc(1, size)) == NULL)
		error("calloc");
	return p;
}

void eselect(int max_fd, fd_set *fds, struct timeval *tv)
{
	if (select(max_fd, fds, NULL, NULL, tv) < 0) {
		if (errno == EINTR)
			eselect(max_fd, fds, tv);
		else
			error("select");
	}
}

void ewrite(int fd, void *buf, int size)
{
	int ret;

	if ((ret = write(fd, buf, size)) < 0)
		error("write");
	else if (ret < size)
		ewrite(fd, (char *) buf + ret, size - ret);
}

void eforkpty(int *master, int lines, int cols)
{
	int slave;
	char *name;
	pid_t pid;

	if ((*master = posix_openpt(O_RDWR | O_NOCTTY)) < 0
		|| grantpt(*master) < 0 || unlockpt(*master) < 0
		|| (name = ptsname(*master)) == NULL)
		error("forkpty");
		
	slave = eopen(name, O_RDWR | O_NOCTTY);
	ioctl(slave, TIOCSWINSZ, &(struct winsize)
		{.ws_row = lines, .ws_col = cols, .ws_xpixel = 0, .ws_ypixel = 0});

	pid = fork();
	if (pid < 0)
		error("fork");
	else if (pid == 0) { /* child */
		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		setsid();
		/* ioctl may fail in Mac : ref http://www.opensource.apple.com/source/Libc/Libc-825.25/util/pty.c?txt */
		ioctl(slave, TIOCSCTTY, NULL);
		close(slave);
		close(*master);
		if (setenv("TERM", term_name, 1) < 0)
			error("setenv");
		if (execlp(shell_cmd, shell_cmd, NULL) < 0)
			error("execl");
	}
	/* parent */
	close(slave);
}

void esigaction(int signo, struct sigaction *act, struct sigaction *oact)
{
	if (sigaction(signo, act, oact) < 0)
		error("sigaction");
}

void etcgetattr(int fd, struct termios *tm)
{
	if (tcgetattr(fd, tm) < 0)
		error("tcgetattr");
}

void etcsetattr(int fd, int action, struct termios *tm)
{
	if (tcsetattr(fd, action, tm) < 0)
		error("tcgetattr");
}
