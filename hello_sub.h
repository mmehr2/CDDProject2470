/*
 * Do the work for chapter 2 homework.
 */

#ifdef HELLO_DOUBLE_MODULE
extern int hmx_hello_greeter(int num_times, char *target);
extern void hmx_goodbye_greeter(char *target);
#else
extern int hello_greeter(int num_times, char *target);
extern void goodbye_greeter(char *target);
#endif
