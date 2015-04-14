#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
/* Use in conjunction with test-kdbus-daemon */

#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "kdbus-test.h"
#include "kdbus-util.h"
#include "kdbus-enum.h"

int kdbus_test_send(struct kdbus_test_env *env)
{
	int ret;
	int serial = 1;
	int fds[3];
	size_t i;

	if (!env->conn)
		return EXIT_FAILURE;

	fds[0] = open("data/file1", O_RDONLY);
	fds[1] = open("data/file2", O_WRONLY);
	fds[2] = open("data/file3", O_RDWR);

	for (i = 0; i < ELEMENTSOF(fds); i++) {
		if (fds[i] < 0) {
			fprintf(stderr, "Unable to open data/fileN file(s)\n");
			return EXIT_FAILURE;
		}
	}

	ret = kdbus_msg_send(env->conn, "com.example.kdbus-test", serial++,
			     0, 0, 0, 0, 0, NULL);
	if (ret < 0)
		fprintf(stderr, "error sending simple message: %d (%m)\n",
			ret);

	ret = kdbus_msg_send(env->conn, "com.example.kdbus-test", serial++,
			     0, 0, 0, 0, 1, fds);
	if (ret < 0)
		fprintf(stderr, "error sending message with 1 fd: %d (%m)\n",
			ret);

	ret = kdbus_msg_send(env->conn, "com.example.kdbus-test", serial++,
			     0, 0, 0, 0, 2, fds);
	if (ret < 0)
		fprintf(stderr, "error sending message with 2 fds: %d (%m)\n",
			ret);

	ret = kdbus_msg_send(env->conn, "com.example.kdbus-test", serial++,
			     0, 0, 0, 0, 3, fds);
	if (ret < 0)
		fprintf(stderr, "error sending message with 3 fds: %d (%m)\n",
			ret);

	for (i = 0; i < ELEMENTSOF(fds); i++)
		close(fds[i]);

	return EXIT_SUCCESS;
}
