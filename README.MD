# ft_ping — Reimplementing `ping`

We are remaking the classic `ping` command. `ping` uses **ICMP** — it does not
rely on TCP or UDP at all.

---

## ️ The OSI 7 Layers

| Layer | Layer Name   | Examples / Protocols     |
| :---: | :----------- | :----------------------- |
| **7** | Application  | HTTP, DNS, SSH           |
| **6** | Presentation | TLS/Encryption, Encoding |
| **5** | Session      | Session Management       |
| **4** | Transport    | TCP, UDP                 |
| **3** | **Network**  | **IP, ICMP**             |
| **2** | Data Link    | Ethernet, Wi-Fi          |
| **1** | Physical     | Cables, Radio            |


---

##  The full packet

An ICMP Echo Request (the "ping") wrapped inside an IP packet:

```
0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Version|  IHL  |Type of Service|          Total Length          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Identification        |Flags|      Fragment Offset     |     IP
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+    HEADER
|  Time to Live | Protocol = 1  |         Header Checksum         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Source Address                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Destination Address                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                 Options (only if IHL > 5)                      |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|     Type      |     Code     |          Checksum              |    ICMP
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   HEADER
|                 Rest of Header (varies by type)               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Data ...                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

---

## 🌐 Layer 1 — The IP Header

Every field, what it does, and why you'd care while implementing `ping`:

| Field                            | Size             | Meaning                                                                                                                                             |
| -------------------------------- | ---------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Version**                      | 4 bits           | `4` for IPv4.                                                                                                                                       |
| **IHL**                          | 4 bits           | Header length in 32-bit words. Usually `5` (→ 20 bytes, no options). Multiply by 4 to get bytes — this is how you find where the ICMP layer starts. |
| **Type of Service**              | 1 byte           | QoS / traffic priority marking. Not something `ping` touches.                                                                                       |
| **Total Length**                 | 2 bytes          | Size of the _entire_ packet (IP header + everything inside), in bytes.                                                                              |
| **Identification**               | 2 bytes          | Used to reassemble fragments if the packet got split up.                                                                                            |
| **Flags / Fragment Offset**      | 3 bits / 13 bits | Also fragmentation bookkeeping.                                                                                                                     |
| **Time to Live (TTL)**           | 1 byte           | Decremented by every router hop; the packet dies at `0`. `traceroute` abuses this to map out routes hop-by-hop.                                     |
| **Protocol**                     | 1 byte           | What's riding inside: `1` = ICMP, `6` = TCP, `17` = UDP.                                                                                            |
| **Header Checksum**              | 2 bytes          | Integrity check — but only over the IP header itself.                                                                                               |
| **Source / Destination Address** | 4 bytes each     | The two IPs involved.                                                                                                                               |
| **Options**                      | variable         | Rarely used. Only present if `IHL > 5`.                                                                                                             |

---

## Layer 2 — The ICMP Header

This is where things get interesting for `ping` specifically.

| Field              | Size     | Meaning                                                                                                       |
| ------------------ | -------- | ------------------------------------------------------------------------------------------------------------- |
| **Type**           | 1 byte   | What kind of message this is (table below).                                                                   |
| **Code**           | 1 byte   | Sub-classification within the Type.                                                                           |
| **Checksum**       | 2 bytes  | One's-complement checksum over the _whole_ ICMP message (header + data) — same algorithm as IP/TCP checksums. |
| **Rest of Header** | 4 bytes  | Meaning depends entirely on Type/Code (see below).                                                            |
| **Data**           | variable | Meaning also depends on Type (see below).                                                                     |

### Common `Type` values

| Type | Name                    | What it means                                                |
| ---- | ----------------------- | ------------------------------------------------------------ |
| `0`  | Echo Reply              | The "pong" you get back.                                     |
| `8`  | Echo Request            | The "ping" you send.                                         |
| `3`  | Destination Unreachable | Packet couldn't be delivered.                                |
| `11` | Time Exceeded           | TTL hit `0` mid-route — this is what `traceroute` relies on. |
| `5`  | Redirect                | "Use a different route for this."                            |

`Type 3` also has sub-codes worth knowing:

| Code | Meaning             |
| ---- | ------------------- |
| `0`  | Network Unreachable |
| `1`  | Host Unreachable    |
| `3`  | Port Unreachable    |

### `Rest of Header` — it depends on the Type

- **Echo Request / Reply:** split into an **Identifier** (2 bytes — usually the
  process ID, used to match replies back to the right `ping` process) and a
  **Sequence Number** (2 bytes — incremented per packet, used to detect drops
  or reordering).
- **Destination Unreachable / Time Exceeded:** this field is unused (zeroed
  out). The real information lives in the `Data` section instead.

### `Data` — it _also_ depends on the Type

- **Echo Request / Reply:** arbitrary payload. Traditionally just filler
  (the classic 32 or 56 bytes) used to pad the packet to a known size and to
  confirm the reply matches what was sent.
- **Error messages** (Destination Unreachable, Time Exceeded, etc.): contains
  a **copy of the original IP header plus the first 8 bytes of the original
  payload** that triggered the error. This is how the original sender figures
  out exactly which outgoing packet caused the problem — meaning an error
  message is really a packet-inside-a-packet:

```
┌───────────────────────────────────────────────────────────┐
│ IP header (the error message's own)                        │
├───────────────────────────────────────────────────────────┤
│ ICMP header  (Type 11 / Type 3, Rest of Header = 0)         │
├───────────────────────────────────────────────────────────┤
│ ↳ copy of the ORIGINAL IP header                            │
│ ↳ first 8 bytes of the ORIGINAL packet's payload            │
│    (for a ping, that's the original ICMP echo header —      │
│     i.e. the real id/sequence that triggered this error)    │
└───────────────────────────────────────────────────────────┘
```

---

## Running ita

Raw sockets need elevated privileges. Either run as root, or grant the
capability directly to the binary:

```bash
sudo setcap cap_net_raw+ep ./ft_ping
```
