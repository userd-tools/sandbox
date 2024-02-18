

/* Headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>
#include <pwd.h>
#include <limits.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>



/* Debug routine defines */
#define NAME         "sampled"
#define RUN_AS_USER  "root"
#define RET_CODE     ((!strcmp("main", __func__)) ? EXIT_FAILURE : -1)

#define OLOG_I(fmt, ...)   fprintf(stdout, " ---- %s %s[%d] %-16s: %03d: "              fmt "\n", gettime(), NAME, getpid(), __func__, __LINE__,                         ## __VA_ARGS__)
#define OLOG_E(fmt, ...)   fprintf(stderr, " ---- %s %s[%d] %-16s: %03d: ERR=%d (%s): " fmt "\n", gettime(), NAME, getpid(), __func__, __LINE__, errno, strerror(errno), ## __VA_ARGS__)

//#define SLOG_I(fmt, ...)   syslog(LOG_INFO, "%-16s: %03d: " fmt "\n",              __func__, __LINE__,                         ## __VA_ARGS__)
#define SLOG_I(fmt, ...)   syslog(LOG_ERR, "%-16s: %03d: " fmt "\n",              __func__, __LINE__,                         ## __VA_ARGS__)
#define SLOG_E(fmt, ...)   syslog(LOG_ERR , "%-16s: %03d: ERR=%d (%s): " fmt "\n", __func__, __LINE__, errno, strerror(errno), ## __VA_ARGS__)

#define LOG_I(fmt, ...) OLOG_I(fmt, ##__VA_ARGS__); SLOG_I(fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...) OLOG_E(fmt, ##__VA_ARGS__); SLOG_E(fmt, ##__VA_ARGS__)

#define INFO(_e)  LOG_I("(%s) == %d", #_e, _e)
#define EXIT(_e)  exit_log(_e); exit(_e)

#define ERR_DO(   _e,          ... )  if ((_e))       { __VA_ARGS__                                                                      ; return RET_CODE; }
#define ERR(      _e, _v           )  if ((_e) == _v) { LOG_E("((%s) == %d); return %d;"              , #_e, _v, RET_CODE               ); return RET_CODE; }
#define ERR_M(    _e, _v, fmt, ... )  if ((_e) == _v) { LOG_E("((%s) == %d); return %d; /* " fmt " */", #_e, _v, RET_CODE, ##__VA_ARGS__); return RET_CODE; }
#define ERR_ON(   _e               )  if ((_e))       { LOG_E("(%s); return %d;"                      , #_e,     RET_CODE               ); return RET_CODE; }
#define ERR_ON_M( _e,     fmt, ... )  if ((_e)      ) { LOG_E("(%s); return %d; /* " fmt " */"        , #_e,     RET_CODE, ##__VA_ARGS__); return RET_CODE; }

#define ERR_POSIX(   _e            )  ERR(   _e, -1                     )
#define ERR_POSIX_M( _e, fmt, ...  )  ERR_M( _e, -1, fmt, ##__VA_ARGS__ )

#define UNUSED(_x) ((_x) = (_x))

/* **** */

char timeline[32];
char *gettime(void)
{
	time_t t; time(&t);
	struct tm *ts; ts = localtime(&t);
	memset(timeline, '\0', 32);
	sprintf(timeline, "%04d.%02d.%02d %02d:%02d:%02d", ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec);
	return timeline;
}


/* helpers */


void log_init(void)
{
	openlog(NAME, LOG_NDELAY | LOG_PID, LOG_DAEMON);
	LOG_I("--------------------");
	LOG_I("init");
}


void log_exit(int status)
{
	LOG_I("exit: %d", status);
	LOG_I("--------------------");
	closelog();
}

// TODO: signalling

int daemonize(void)
{
	/* Drop mask */
	umask(0);
	
	/* Set root */
	if (getuid() != 0) {
		struct passwd *pw = getpwnam(RUN_AS_USER);
		if (pw) {
			ERR_POSIX(setuid(pw->pw_uid));
		}
	}
	
	/* Fork for daemon */
	pid_t pid;
	ERR_POSIX(pid = fork());
	if (pid) {
		SLOG_I("close parent: %d", pid);
		exit(EXIT_SUCCESS);
	}
	
	/* Set session ID for leaving terminal */
	pid_t sid;
	ERR_POSIX(sid = setsid());
	SLOG_I("set session id: %d", sid);
	
	/* Fork again to disable terminal access tries */
	ERR_POSIX(pid = fork());
	if (pid) {
		SLOG_I("close parent after fork: %d", pid);
		exit(EXIT_SUCCESS);
	}
	
	ERR_POSIX(chdir("/"));
	
	struct rlimit rl;
	ERR_POSIX(getrlimit(RLIMIT_NOFILE, &rl));
	if (rl.rlim_max == RLIM_INFINITY) {
		rl.rlim_max = 1024;
	}
	
	SLOG_I("close file descriptors and attach 0/1/2 to /dev/null");
	closelog();
	
	unsigned int i;
	for (i = 0; i < rl.rlim_max; i++) {
		close(i);
	}
	
	int fd_i, fd_o, fd_e;
	ERR_POSIX(fd_i = open("/dev/null", O_RDWR));
	ERR_POSIX(fd_o = dup(STDIN_FILENO));
	ERR_POSIX(fd_e = dup(STDIN_FILENO));
	ERR_ON(fd_i != 0 || fd_o != 1 || fd_e != 2);
	openlog(NAME, LOG_NDELAY | LOG_PID, LOG_DAEMON);
	
	return 0;
}


/* entry point */

int main(int argc, const char *argv[])
{
	log_init();
	
	LOG_I("starting");
	
	LOG_I("init daemon routine: detaching from terminal");
	ERR_POSIX(daemonize());
	/*
	LOG_I("init signals handling");
	int signal_fd;
	ERR_POSIX(signal_fd = signalling());
	*/
	
	/* Now loop */
	int signal = 0;
	while (!signal) {
		SLOG_I("looping for signal");
		/* Block until there is something to be done */
		
		#if 0
		/* Signal received? */
		if (fds[FD_POLL_SIGNAL].revents & POLLIN) {
			struct signalfd_siginfo fdsi;
			ERR_ON(read(fds[FD_POLL_SIGNAL].fd, &fdsi, sizeof(fdsi)) != sizeof(fdsi));
			
			/* Check signal */
			switch (fdsi.ssi_signo) {
				case SIGINT:
					signal = SIGINT;
					continue;
					break;
				case SIGTERM:
					signal = SIGTERM;
					continue;
					break;
				case SIGCHLD:
					wait(NULL);
					break;
				default:
					LOG_I("unexpected signal received: %d", fdsi.ssi_signo);
					break;
			}
		}
		#endif
		sleep(10);
		// system("sudo -u USER open -n /Applications/Name.app");
		sleep(5);
		signal = SIGINT;
		/* DO SOMETHING HERE */
		/* receive / process events */
		
	}
	
	LOG_I("exit by signal==%d; cleaning", signal);
	//close(signal_fd);
	log_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}


