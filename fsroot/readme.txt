SimpleOS
========

This file lives on the SimpleFS (SFS1) volume, read from the primary IDE
disk with a polled ATA PIO driver.

Try:
  ls
  cat motd.txt
  stat readme.txt
  df
  touch notes.txt
  write notes.txt hello from the shell
  cat notes.txt
  rm notes.txt

Files you create with touch/write persist across reboots because they are
written back to the disk image.
