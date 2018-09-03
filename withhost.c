#define _GNU_SOURCE  

#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

static char *help_text = "usage: "PACKAGE_NAME" [-h <host>] [--export] "
"[--version] [--help] -- [prog]\n"
"\n"
"Overrides host lookups for a given command.\n"
"\n"
"withhost can be used to override the response to a DNS lookup for a given\n"
"set of domains. Functionally, it is equivalent to adding /etc/hosts entries\n"
"manually, except it is local for a given command.\n"
"\n"
"For setuid executables, withhost must be invoked with root privileges,\n"
"otherwise it will not work.\n"
"\n"
"Examples:\n"
"$ withhost --host example.com=10.0.0.1 -- getent hosts 10.0.0.1\n"
"10.0.0.1        example.com\n"
"$ sudo withhost --host example.com=10.0.0.1 -- ping example.com\n"
"PING example.com (10.0.0.1) 56(84) bytes of data.\n"
"^C\n"
"$ eval $(withhost --host example.com 10.0.0.1 --export)\n"
"$ getent hosts 10.0.0.1\n"
"10.0.0.1        example.com\n"
"\n"
"optional arguments:\n"
"  -h <host>, --host <host>  specifies a host entry, in the format "
"${hostname}=${ip_address}\n"
"  --export                  write environment variables to standard output\n"
"  --version                 write version string to standard output\n"
"  --help                    show this help message and exit\n";
static int flag_version;
static int flag_help;
static int flag_export;
static struct option long_options[] = {
  {"host", required_argument, NULL, 'h'},
  {"version", no_argument, &flag_version, 1},
  {"export", no_argument, &flag_export, 1},
  {"help", no_argument, &flag_help, 1},
  {NULL, 0, NULL, 0}
};

int main(int argc, char **argv) {
  char *host_env = NULL;

  int c;
  while ((c = getopt_long(argc, argv, "h:", long_options, NULL)) != -1) {
    switch (c) {
      case 'h':
        if (host_env) {
          char *new_env;
          asprintf(&new_env, "%s#%s", host_env, optarg);
          free(host_env);
          host_env = new_env;
        } else {
          host_env = strdup(optarg);
        }

        break;
    }
  }

  if (flag_version) {
    printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
    return 0;
  }

  if (flag_help) {
    printf(help_text);
    return 0;
  }

  if (!host_env) {
    errx(EXIT_FAILURE, "must specify at least one --host");
  }

  if (setenv("LD_WITHHOST_OVERRIDES", host_env, true)) {
    err(EXIT_FAILURE, "cannot set LD_WITHHOST_OVERRIDES env");
  }

  char *preload_path = strdup(LIBDIR"/libwithhost.so");
  if (access(preload_path, F_OK) == -1) {
    errx(EXIT_FAILURE, "cannot find libwithhost, expected at '%s'",
         preload_path);
  }

  // Forward LD_PRELOAD in case any application is relying on it.
  char *ld_preload = getenv("LD_PRELOAD");
  if (!ld_preload) {
    ld_preload = preload_path;
  } else {
    asprintf(&ld_preload, "%s %s", preload_path, ld_preload);
  }

  if (setenv("LD_PRELOAD", ld_preload, true)) {
    err(EXIT_FAILURE, "cannot set LD_PRELOAD env");
  }

  if (flag_export) {
    printf("LD_PRELOAD=%s\n", ld_preload);
    printf("LD_WITHHOST_OVERRIDES=%s\n", host_env);
    return 0;
  }

  free(ld_preload);

  int args_start = 0;
  for (; args_start < argc; args_start++) {
    if (!strcmp(argv[args_start], "--")) {
      break;
    }
  }

  if (++args_start < argc) {
    char *prog = argv[args_start];
    execvp(prog, argv + args_start);
    err(EXIT_FAILURE, "exec '%s' failed", prog);
  } else {
    errx(EXIT_FAILURE, "not invoking any program");
  }
}
