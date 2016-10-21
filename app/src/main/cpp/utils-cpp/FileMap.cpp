/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Shared file mapping class.
//

#define LOG_TAG "filemap"
#include "../fakeLog.h"

#include "../utils/FileMap.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#include <string.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>

using namespace android;

/*static*/ long FileMap::mPageSize = -1;


/*
 * Constructor.  Create an empty object.
 */
FileMap::FileMap(void)
    : mRefCount(1), mFileName(NULL), mBasePtr(NULL), mBaseLength(0),
      mDataPtr(NULL), mDataLength(0)
{
}

/*
 * Destructor.
 */
FileMap::~FileMap(void)
{
    assert(mRefCount == 0);

    mRefCount = -100;       // help catch double-free
    if (mFileName != NULL) {
        free(mFileName);
    }

    if (mBasePtr && munmap(mBasePtr, mBaseLength) != 0) {
        LOGD("munmap(%p, %d) failed\n", mBasePtr, (int) mBaseLength);
    }

}


/*
 * Create a new mapping on an open file.
 *
 * Closing the file descriptor does not unmap the pages, so we don't
 * claim ownership of the fd.
 *
 * Returns "false" on failure.
 */
bool FileMap::create(const char* origFileName, int fd, off64_t offset, size_t length,
        bool readOnly)
{
    int     prot, flags, adjust;
    off64_t adjOffset;
    size_t  adjLength;

    void* ptr;

    assert(mRefCount == 1);
    assert(fd >= 0);
    assert(offset >= 0);
    assert(length > 0);

    /* init on first use */
    if (mPageSize == -1) {
#if NOT_USING_KLIBC
        mPageSize = sysconf(_SC_PAGESIZE);
        if (mPageSize == -1) {
            LOGE("could not get _SC_PAGESIZE\n");
            return false;
        }
#else
        /* this holds for Linux, Darwin, Cygwin, and doesn't pain the ARM */
        mPageSize = 4096;
#endif
    }

    adjust   = offset % mPageSize;
try_again:
    adjOffset = offset - adjust;
    adjLength = length + adjust;

    flags = MAP_SHARED;
    prot = PROT_READ;
    if (!readOnly)
        prot |= PROT_WRITE;

    ptr = mmap(NULL, adjLength, prot, flags, fd, adjOffset);
    if (ptr == MAP_FAILED) {
    	// Cygwin does not seem to like file mapping files from an offset.
    	// So if we fail, try again with offset zero
    	if (adjOffset > 0) {
    		adjust = offset;
    		goto try_again;
    	}

        LOGE("mmap(%ld,%ld) failed: %s\n",
            (long) adjOffset, (long) adjLength, strerror(errno));
        return false;
    }
    mBasePtr = ptr;

    mFileName = origFileName != NULL ? strdup(origFileName) : NULL;
    mBaseLength = adjLength;
    mDataOffset = offset;
    mDataPtr = (char*) mBasePtr + adjust;
    mDataLength = length;

    assert(mBasePtr != NULL);

    LOGV("MAP: base %p/%d data %p/%d\n",
        mBasePtr, (int) mBaseLength, mDataPtr, (int) mDataLength);

    return true;
}

/*
 * Provide guidance to the system.
 */
int FileMap::advise(MapAdvice advice)
{
#if HAVE_MADVISE
    int cc, sysAdvice;

    switch (advice) {
        case NORMAL:        sysAdvice = MADV_NORMAL;        break;
        case RANDOM:        sysAdvice = MADV_RANDOM;        break;
        case SEQUENTIAL:    sysAdvice = MADV_SEQUENTIAL;    break;
        case WILLNEED:      sysAdvice = MADV_WILLNEED;      break;
        case DONTNEED:      sysAdvice = MADV_DONTNEED;      break;
        default:
                            assert(false);
                            return -1;
    }

    cc = madvise(mBasePtr, mBaseLength, sysAdvice);
    if (cc != 0)
        LOGW("madvise(%d) failed: %s\n", sysAdvice, strerror(errno));
    return cc;
#else
	return -1;
#endif // HAVE_MADVISE
}
