# withhost [![Linux Build Status](https://travis-ci.org/Xyene/withhost.svg?branch=master)](https://travis-ci.org/Xyene/withhost)

`withhost` is a utility that allows overriding DNS lookups for a given command invocation. It works on Linux and FreeBSD.

Some potential use-cases include accessing backend nodes behind a load balancer, checking whether a service is responding correctly after porting it to a new server (before switching the DNS over), etc.

## Usage
```
usage: withhost [-h <host>] [--export] [--version] [--help] -- [<program> ...]

Overrides host lookups for a given command.

withhost can be used to override the response to a DNS lookup for a given
set of domains. Functionally, it is equivalent to adding /etc/hosts entries
manually, except it is local for a given command. That is, /etc/hosts
should be prioritized over DNS responses in /etc/nsswitch.conf for this
utility to work well.

For setuid executables, withhost must be invoked with root privileges,
otherwise it will not work.

Examples:
$ withhost --host example.com=10.0.0.1 -- getent hosts 10.0.0.1
10.0.0.1        example.com
$ sudo withhost --host example.com=10.0.0.1 -- ping example.com
PING example.com (10.0.0.1) 56(84) bytes of data.
^C
$ eval $(withhost --host example.com=10.0.0.1 --export)
$ getent hosts 10.0.0.1
10.0.0.1        example.com

optional arguments:
  -h <host>, --host <host>  specifies a host entry, in the format ${hostname}=${ip_address}
  --export                  write environment variables to standard output
  --version                 write version string to standard output
  --help                    show this help message and exit
```

## Installation

You will need an `autotools` install.

```bash
$ git clone https://github.com/Xyene/withhost.git
$ cd withhost
$ autoreconf -i
$ ./configure
$ make
$ make install
```

## Miscellaneous

### `/etc/nsswitch.conf` resolution order
For `withhost` to work correctly, `files` must be prioritized over everything in the `hosts` entry of `/etc/nsswitch.conf`. On default installations, it usually is. Otherwise, a DNS response may be trusted over the entry in `/etc/hosts`, making `withhost` useful only for defining non-existent (e.g. NXDOMAIN) hosts.

### Working with `setuid` executables
`setuid` executables will generally ignore `LD_PRELOAD`, which `withhost` uses to provide its functionality. Notably, `ping` does this to open raw sockets. `withhost` must be invoked as root when running these kinds of executables.
