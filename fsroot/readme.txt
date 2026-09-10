SimpleOS
========

You are talking to /sh, a user-mode shell (an ELF program on this
SimpleFS volume). Every command except 'help', 'exit' and 'reboot' is
another program loaded from the filesystem.

Try:
  ls
  cat motd.txt
  hexdump fortune.txt
  free
  echo hello there
  write notes.txt a line of text
  cat notes.txt
  rm notes.txt

Files created with write persist across reboots because they are
written straight back to the disk image.
