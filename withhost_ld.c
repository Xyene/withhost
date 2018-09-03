#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dlfcn.h>

#define _ETC_HOSTS "/etc/hosts"

struct host_override {
  char *from_pattern;
  char *from_id;
  char *to;
};

typedef struct host_override host_override_t;
host_override_t *host_overrides[65536];
int num_host_overrides = 0;

char *hosts_path;

typedef FILE* (*fopen_t)(const char *pathname, const char *mode);
fopen_t real_fopen; 

typedef int (*getaddrinfo_t)(const char *node, const char *service,
                             const struct addrinfo *hints,
                             struct addrinfo **res);
getaddrinfo_t real_getaddrinfo;

__attribute__((destructor)) static void cleanup() {
  if (hosts_path) {
    remove(hosts_path);
  }
}

void add_route_override(host_override_t *route) {
  host_overrides[num_host_overrides++] = route;
  printf("Added route from %s to %s\n", route->from_pattern, route->to);
}

bool is_valid_address(const char *buf) {
  char ip_buf[INET6_ADDRSTRLEN];
  return inet_pton(AF_INET, buf, &ip_buf) == 1 ||
         inet_pton(AF_INET6, buf, &ip_buf) == 1;
}

char *make_patched_hosts_file() {

}

__attribute__((constructor)) static void setup() {
  real_fopen = dlsym(RTLD_NEXT, "fopen");
  real_getaddrinfo = dlsym(RTLD_NEXT, "getaddrinfo");

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

    host_override_t *route = malloc(sizeof(host_override_t));
    route->from_pattern = from;
    route->to = to;

    add_route_override(route);

    fprintf(hosts_file, "%s  %s\n", to, from);
    free(host);

    mapping = strtok(NULL, "#");
  }

  free(host_overrides);

  // Append the real /etc/hosts file.
  FILE *real_hosts_file = real_fopen(_ETC_HOSTS, "r");
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

FILE *fopen(const char *pathname, const char *mode) {
  // If the call is to open /etc/hosts, redirect it to point to our patched
  // hosts file. Note that when we need to fopen /etc/hosts for reading, we do
  // so directly with |real_fopen|.
  if (!strcmp(pathname, _ETC_HOSTS)) {
    pathname = hosts_path;
  }
  
  return real_fopen(pathname, mode);
}
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res) {
/*  for (int i = 0; i < nroutes; i++) {
    if (!strcmp(routes[i]->from, node)) {
      printf("rerouting %s to %s\n", routes[i]->from, routes[i]->to);
      struct addrinfo *ret = malloc(sizeof(struct addrinfo));
      ret->ai_flags = 0;
      ret->ai_family = AF_INET;
      ret->ai_socktype = SOCK_STREAM;
      ret->ai_protocol = IPPROTO_TCP;
      printf("===>%d\n", hints->ai_protocol);
      ret->ai_flags = 0;
      ret->ai_addrlen = sizeof(struct sockaddr_in);
      struct sockaddr_in *addr = malloc(ret->ai_addrlen);
      ret->ai_addr = (struct sockaddr *) addr;
      
      addr->sin_family = AF_INET;
      addr->sin_port = 0;
      inet_aton(routes[i]->to, &(addr->sin_addr));

      ret->ai_canonname = NULL;
      ret->ai_next = NULL;

      // freeaddrinfo((*res)->ai_next);
      //(*res)->ai_next = NULL;

      // (*res)->ai_addrlen = strlen(routes[i]->to_resolv);
//      strcpy((*res)->ai_addr->sa_data, routes[i]->to_resolv);
      
      *res = ret;
      return 0;
    }
  }
*/
  getaddrinfo_t hook = dlsym(RTLD_NEXT, "getaddrinfo");
  int ret = hook(node, service, hints, res);
  return ret;
}
