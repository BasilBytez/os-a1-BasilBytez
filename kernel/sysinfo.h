#ifndef SYSINFO_H
#define SYSINFO_H

// This structure holds system statistics and is copied to user space.
struct sysinfo {
  uint64 freemem;   // bytes of free physical memory
  uint64 nproc;     // processes whose state is not UNUSED
  uint64 nopenfile; // entries in the system file table with ref > 0[cite: 1]
};

#endif
