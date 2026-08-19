# ft_traceroute

Traceroute implementation, using raw ICMP or UDP probes.
## Build

```sh
make
```

Requires root (or `cap_net_raw`) to open raw sockets.

## Usage

```sh
sudo ./ft_traceroute [ -InrV ] [ -m max_ttl ] [ -q nqueries ] [ -z sendwait ] host
```

| Flag | Description |
|------|-------------|
| `-I` | Use ICMP ECHO instead of UDP |
| `-m max_ttl` | Max hops to probe (default 30) |
| `-q nqueries` | Probes per hop (default 3) |
| `-z sendwait` | Minimum delay between probes, ms (default 10) |
| `-n` | Skip reverse DNS lookup |
| `-r` | Bypass normal routing, send to a directly attached host |
| `-V` | Print version |

## How it works

Probes are sent with increasing TTL, and ICMP `TIME_EXCEEDED` / `DEST_UNREACHABLE` replies are matched back to a probe via its sequence number (ICMP) or destination port (UDP).

## Author

tfreydie