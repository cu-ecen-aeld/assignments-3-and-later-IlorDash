#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT "9000"
#define BACKLOG 10
#define AESD_SOCKET_DATA "/var/tmp/aesdsocketdata"
#define BUF_SIZE 1000

int is_exit, accept_conn;
int sock_fd;

static void sig_handler(int signo) {

	switch (signo) {
		case SIGINT:
			syslog(LOG_ERR, "Caught signal, exiting\n");
			break;
		case SIGTERM:
			syslog(LOG_ERR, "Caught signal, exiting\n");
			break;
		default:
			syslog(LOG_ERR, "Caught unknown SIGNAL\n");
			return;
	}

	if (accept_conn) {
		is_exit = 1;
	}

	shutdown(sock_fd, SHUT_RDWR);
	close(sock_fd);
	remove(AESD_SOCKET_DATA);
	exit(0);
}

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int recv_lines(int fd, char *buf, int size) {

	int recv_num = 0;
	memset(buf, 0, size);

	do
		recv_num = recv(fd, buf, size, 0);
	while ((buf[recv_num - 1] != '\n') && (recv_num != 0));

	return recv_num;
}

int main(int argc, char *argv[]) {

	int client_fd, write_fd, read_fd;
	struct addrinfo hints, *servinfo;
	struct sockaddr_storage sock_in_addr;
	socklen_t sock_in_size;
	char conn_ip_addr[INET6_ADDRSTRLEN];
	int yes = 1;
	int res;
	char buf[BUF_SIZE];
	ssize_t bytes_read;
	pid_t pid;

	is_exit = 0;

	openlog("aesdsocket", LOG_PID, LOG_USER);
	syslog(LOG_INFO, "Starting aesdsocket\n");

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	memset(buf, 0, BUF_SIZE);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((res = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
		goto err_free;
	}

	if ((sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
		perror("server: socket");
		goto err_free_close;
	}

	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		perror("setsockopt");
		goto err_free_close;
	}

	if (bind(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		close(sock_fd);
		perror("server: bind");
		goto err_free_close;
	}

	freeaddrinfo(servinfo);

	if (argc > 1 && strcmp(argv[1], "-d") == 0)
		switch (pid = fork()) {
			case -1:
				perror("fork");
				exit(1);
			case 0:
				break;
			default:
				return 0;
		}

	if (listen(sock_fd, BACKLOG) != 0) {
		perror("listen");
		return -1;
	}

	while (!is_exit) {
		sock_in_size = sizeof(sock_in_addr);
		client_fd = accept(sock_fd, (struct sockaddr *)&sock_in_addr, &sock_in_size);
		if (client_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(sock_in_addr.ss_family, get_in_addr((struct sockaddr *)&sock_in_addr), conn_ip_addr, sizeof(conn_ip_addr));
		syslog(LOG_INFO, "Accepted connection from %s\n", conn_ip_addr);

		if ((write_fd = open(AESD_SOCKET_DATA, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1) {
			syslog(LOG_ERR, "failed to open: %s\n", AESD_SOCKET_DATA);
			return -1;
		}

		read_fd = open(AESD_SOCKET_DATA, O_RDONLY);
		if (read_fd == -1) {
			syslog(LOG_ERR, "failed to open: %s\n", AESD_SOCKET_DATA);
			return -1;
		}

		while ((bytes_read = recv(client_fd, buf, BUF_SIZE, 0))) {
			write(write_fd, buf, bytes_read);
			if (buf[strlen(buf) - 1] == '\n') {
				while ((bytes_read = read(read_fd, buf, BUF_SIZE)) > 0) {
					if (send(client_fd, buf, bytes_read, 0) == -1) {
						perror("send");
						syslog(LOG_ERR, "send failed\n");
						break;
					}
				}
			}
			memset(buf, 0, BUF_SIZE);
		}

		res = close(write_fd);
		res = close(read_fd);
		res = close(client_fd);

		syslog(LOG_ERR, "Closed connection from %s\n", conn_ip_addr);
		accept_conn = 0;
	}

	shutdown(sock_fd, SHUT_RDWR);
	close(sock_fd);
	remove(AESD_SOCKET_DATA);
	return 0;

err_free_close:
	shutdown(sock_fd, SHUT_RDWR);
	close(sock_fd);
err_free:
	freeaddrinfo(servinfo);
	return -1;
}