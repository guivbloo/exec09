#ifndef UTILSTIME_H
#define UTILSTIME_H

void init_time (void);
long get_elapsed_realtime (void);
long time_diff (struct timeval *old, struct timeval *new);

#endif /* UTILSTIME_H */
