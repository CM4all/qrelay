/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#ifndef MOUNT_PROC_HXX
#define MOUNT_PROC_HXX

#include <string>

struct MountInfo {
    /**
     * The relative path inside the file system which was mounted on
     * the given mount point.  This is relevant for bind mounts.
     */
    std::string root;

    /**
     * The filesystem type.
     */
    std::string filesystem;

    /**
     * The device which was mounted on the given mount point.
     */
    std::string source;

    bool IsDefined() const {
        return !root.empty() || !source.empty();
    }
};

/**
 * Determine which file system is mounted at the given mount point
 * path (exact match required).
 *
 * Throws std::runtime_error on error.
 */
MountInfo
ReadProcessMount(unsigned pid, const char *mountpoint);

#endif
