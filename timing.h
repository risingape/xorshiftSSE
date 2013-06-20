

#include <time.h>

/* compute the difference of two timespec values */
struct timespec diff(struct timespec start, struct timespec end) {
	struct timespec tmp;

	if ((end.tv_nsec - start.tv_nsec) < 0) {
		tmp.tv_sec = end.tv_sec - start.tv_sec - 1;
		tmp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	} else {
		tmp.tv_sec = end.tv_sec - start.tv_sec;
		tmp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}

	return (tmp);
}
