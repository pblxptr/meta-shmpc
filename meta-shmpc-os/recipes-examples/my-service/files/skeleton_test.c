#include <unistd.h>
#include <syslog.h>

#define DAEMON_NAME "simpledaemon"

int main(int argc, char *argv[])
{
	setlogmask(LOG_UPTO(LOG_INFO));
	openlog(DAEMON_NAME, LOG_CONS | LOG_PERROR, LOG_USER);

	daemon(0, 0);

	while (1) {
		syslog(LOG_INFO, "daemon running");
		sleep(30);
	}
	return 0;
}
