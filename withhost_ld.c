#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <dlfcn.h>
#include <err.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#define _ETC_HOSTS "/etc/hosts"

char *hosts_path;

typedef FILE *(*fopen_t)(const char *pathname, const char *mode);
fopen_t real_fopen;

FILE *do_fopen(const char *pathname, const char *mode) {
  // This wrapper is necessary since sometimes `fopen` is called before the
  // constructor, so we have two separate points where this lookup might need
  // to be performed.
  if (!real_fopen) {
    real_fopen = dlsym(RTLD_NEXT, "fopen");
  }

  return real_fopen(pathname, mode);
}

bool is_valid_address(const char *buf) {
  char ip_buf[INET6_ADDRSTRLEN];
  return inet_pton(AF_INET, buf, &ip_buf) == 1 ||
         inet_pton(AF_INET6, buf, &ip_buf) == 1;
}

FILE *fopen(const char *pathname, const char *mode) {
  // If the call is to open /etc/hosts, redirect it to point to our patched
  // hosts file. Note that when we need to fopen /etc/hosts for reading, we do
  // so directly with |do_fopen|.
  if (hosts_path && !strcmp(pathname, _ETC_HOSTS)) {
    pathname = hosts_path;
  }

  return do_fopen(pathname, mode);
}

__attribute__((constructor)) static void setup(void) {
  char *env_overrides = getenv("LD_WITHHOST_OVERRIDES");
  if (!env_overrides) {
    return;
  }

  char *host_overrides = strdup(env_overrides);

  hosts_path = strdup("/tmp/withhostXXXXXX");
  FILE *hosts_file = fdopen(mkstemp(hosts_path), "w");

  char *mapping = strtok(host_overrides, "#");
  while (mapping != NULL) {
    char *host = strdup(mapping);

    char *sep = host;
    while (*sep != '=' && *sep) {
      sep++;
    }

    if (*sep != '=') {
      errx(EXIT_FAILURE, "no route assignment '=' in '%s'", host);
    }

    // Split up |host| into two strings on the '='.
    *sep = '\0';
    char *from = strdup(host);
    char *to = strdup(sep + 1);

    if (!is_valid_address(to)) {
      errx(EXIT_FAILURE, "invalid IPv4 or IPv6 address: %s", to);
    }

    fprintf(hosts_file, "%s  %s\n", to, from);
    free(host);

    mapping = strtok(NULL, "#");
  }

  free(host_overrides);

  // Append the real /etc/hosts file.
  FILE *real_hosts_file = do_fopen(_ETC_HOSTS, "r");
  if (!real_hosts_file) {
    warnx("could not read /etc/hosts");
    goto end;
  }

  char buf[4096];
  while (fgets(buf, sizeof(buf), real_hosts_file)) {
    fprintf(hosts_file, "%s", buf);
  }

end:
  fclose(hosts_file);
}

__attribute__((destructor)) static void cleanup(void) {
  if (hosts_path) {
    remove(hosts_path);
  }
}
