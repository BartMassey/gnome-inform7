// $Id: memory.h,v 1.2 2009-04-23 21:19:48 pchimento Exp $
// Functions and macros for accessing game memory.

#ifndef GIT_MEMORY_H
#define GIT_MEMORY_H

#include "config.h"

// --------------------------------------------------------------
// Macros for reading and writing big-endian data.

#ifdef USE_BIG_ENDIAN_UNALIGNED
// We're on a big-endian platform which can handle unaligned
// accesses, such as the PowerPC. This means we can read and
// write multi-byte values in glulx memory directly, without
// having to pack and unpack each byte.

#define read32(ptr)    (*((git_uint32*)(ptr)))
#define read16(ptr)    (*((git_uint16*)(ptr)))
#define write32(ptr,v) (read32(ptr)=(git_uint32)(v))
#define write16(ptr,v) (read16(ptr)=(git_uint16)(v))

#else
// We're on a little-endian platform, such as the x86, or a
// big-endian platform that doesn't like unaligned accesses,
// such as the 68K. This means we have to read and write the
// slow and tedious way.

#define read32(ptr)    \
  ( (git_uint32)(((git_uint8 *)(ptr))[0] << 24) \
  | (git_uint32)(((git_uint8 *)(ptr))[1] << 16) \
  | (git_uint32)(((git_uint8 *)(ptr))[2] << 8)  \
  | (git_uint32)(((git_uint8 *)(ptr))[3]))
#define read16(ptr)    \
  ( (git_uint16)(((git_uint8 *)(ptr))[0] << 8)  \
  | (git_uint16)(((git_uint8 *)(ptr))[1]))

#define write32(ptr, v)   \
  (((ptr)[0] = (git_uint8)(((git_uint32)(v)) >> 24)), \
   ((ptr)[1] = (git_uint8)(((git_uint32)(v)) >> 16)), \
   ((ptr)[2] = (git_uint8)(((git_uint32)(v)) >> 8)),  \
   ((ptr)[3] = (git_uint8)(((git_uint32)(v)))))
#define write16(ptr, v)   \
  (((ptr)[0] = (git_uint8)(((git_uint32)(v)) >> 8)),  \
   ((ptr)[1] = (git_uint8)(((git_uint32)(v)))))

#endif // USE_BIG_ENDIAN_UNALIGNED

// Accessing single bytes is easy on any platform.

#define read8(ptr)     (*((git_uint8*)(ptr)))
#define write8(ptr, v) (read8(ptr)=(git_uint8)(v))

// --------------------------------------------------------------
// Globals

extern git_uint32 gRamStart; // The start of RAM.
extern git_uint32 gExtStart; // The start of extended memory (initialised to zero).
extern git_uint32 gEndMem;   // The current end of memory.
extern git_uint32 gOriginalEndMem; // The value of EndMem when the game was first loaded.

// This is the entire gamefile, as read-only memory. It contains
// both the ROM, which is constant for the entire run of the program,
// and the original RAM, which is useful for checking what's changed
// when saving to disk or remembering a position for UNDO.
extern const git_uint8 * gRom;

// This is the current contents of RAM. This pointer actually points
// to the start of ROM, so that you don't have to keep adding and
// subtracting gRamStart, but don't try to access ROM via this pointer.
extern git_uint8 * gRam;

// --------------------------------------------------------------
// Functions

// Initialise game memory. This sets up all the global variables
// declared above. Note that it does *not* copy the given memory
// image: it must be valid for the lifetime of the program.

extern void initMemory (const git_uint8 * game, git_uint32 gameSize);

// Verifies the gamefile based on its checksum. 0 on success, 1 on failure.

extern int verifyMemory ();

// Resizes the game's memory. Returns 0 on success, 1 on failure.

extern int resizeMemory (git_uint32 newSize, int isInternal);

// Resets memory to its initial state. Call this when the game restarts.

extern void resetMemory (git_uint32 protectPos, git_uint32 protectSize);

// Disposes of all the data structures allocated in initMemory().

extern void shutdownMemory ();

// Utility functions -- these just pass an appropriate
// string to fatalError().

extern git_uint32 memReadError (git_uint32 address);
extern void memWriteError (git_uint32 address);

// Functions for reading and writing game memory.

GIT_INLINE git_uint32 memRead32 (git_uint32 address)
{
	if (address <= gRamStart - 4)
		return read32 (gRom + address);
	else if (address <= gEndMem - 4)
		return read32 (gRam + address);
    else
        return memReadError (address);
}

GIT_INLINE git_uint32 memRead16 (git_uint32 address)
{
	if (address <= gRamStart - 4)
		return read16 (gRom + address);
	else if (address <= gEndMem - 2)
		return read16 (gRam + address);
    else
        return memReadError (address);
}

GIT_INLINE git_uint32 memRead8 (git_uint32 address)
{
    if (address <= gRamStart - 4)
        return read8 (gRom + address);
    else if (address < gEndMem)
        return read8 (gRam + address);
    else
        return memReadError (address);
}

GIT_INLINE void memWrite32 (git_uint32 address, git_uint32 val)
{
	if (address >= gRamStart && address <= (gEndMem - 4))
		write32 (gRam + address, val);
	else
        memWriteError (address);
}

GIT_INLINE void memWrite16 (git_uint32 address, git_uint32 val)
{
	if (address >= gRamStart && address <= (gEndMem - 2))
		write16 (gRam + address, val);
	else
        memWriteError (address);
}

GIT_INLINE void memWrite8 (git_uint32 address, git_uint32 val)
{
	if (address >= gRamStart && address < gEndMem)
		write8 (gRam + address, val);
	else
        memWriteError (address);
}

#endif // GIT_MEMORY_H
