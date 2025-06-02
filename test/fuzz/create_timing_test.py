#!/usr/bin/env python3
import struct

# Create a test file with timing info for fuzz3 (LWIP_FUZZ_MULTIPACKET_TIME)
with open('test_timing.bin', 'wb') as f:
    # Packet 1: delay=0ms, SYN packet
    f.write(struct.pack('!I', 0))  # 0ms delay
    f.write(struct.pack('!H', 74)) # 74 bytes packet length

    # Read the original SYN packet
    with open('inputs/tcp/tcp_syn.bin', 'rb') as syn:
        f.write(syn.read())

    # Packet 2: delay=200ms, same packet again (simulating retransmit)
    f.write(struct.pack('!I', 200)) # 200ms delay
    f.write(struct.pack('!H', 74))  # 74 bytes packet length

    # Same SYN packet again
    with open('inputs/tcp/tcp_syn.bin', 'rb') as syn:
        f.write(syn.read())

print('Created test_timing.bin with timing structure')
