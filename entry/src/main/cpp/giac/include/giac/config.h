// minimal config.h for building giac core without autotools
#ifndef GIAC_CONFIG_H
#define GIAC_CONFIG_H


#define HAVE_PTHREAD_H 1
// no GUI, no optional deps
#undef HAVE_LIBMPFR
#undef HAVE_LIBMPFI
#undef HAVE_GSL
#undef HAVE_LIBNauty
#undef HAVE_LIBPTHREAD
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

#endif

#define VERSION "1.9.0"
#define GIAC_VERSION "1.9.0"
