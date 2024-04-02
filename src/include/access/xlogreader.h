/*-------------------------------------------------------------------------
 *
 * xlogreader.h
 *		Definitions for the generic XLog reading facility
 *
 * Portions Copyright (c) 2013-2014, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		src/include/access/xlogreader.h
 *
 * NOTES
 *		See the definition of the XLogReaderState struct for instructions on
 *		how to use the XLogReader infrastructure.
 *
 *		The basic idea is to allocate an XLogReaderState via
 *		XLogReaderAllocate(), and call XLogReadRecord() until it returns NULL.
 *-------------------------------------------------------------------------
 */
#ifndef XLOGREADER_H
#define XLOGREADER_H

#include "access/xlog_internal.h"

typedef struct XLogReaderState XLogReaderState;

/* Function type definition for the read_page callback */
typedef int (*XLogPageReadCB) (XLogReaderState *xlogreader,
										   XLogRecPtr targetPagePtr,
										   int reqLen,
										   XLogRecPtr targetRecPtr,
										   char *readBuf,
										   TimeLineID *pageTLI);
// #define XL_ROUTINE(...) &(XLogReaderRoutine){__VA_ARGS__}
// typedef uint16 RepOriginId;
// typedef Oid RelFileNumber;
// typedef struct RelFileLocator
// {
//     Oid			spcOid;			/* tablespace */
//     Oid			dbOid;			/* database */
//     RelFileNumber relNumber;	/* relation */
// } RelFileLocator;

// typedef struct
// {
//     /* Is this block ref in use? */
//     bool		in_use;

//     /* Identify the block this refers to */
//     RelFileLocator rlocator;
//     ForkNumber	forknum;
//     BlockNumber blkno;

//     /* Prefetching workspace. */
//     Buffer		prefetch_buffer;

//     /* copy of the fork_flags field from the XLogRecordBlockHeader */
//     uint8		flags;

//     /* Information on full-page image, if any */
//     bool		has_image;		/* has image, even for consistency checking */
//     bool		apply_image;	/* has image that should be restored */
//     char	   *bkp_image;
//     uint16		hole_offset;
//     uint16		hole_length;
//     uint16		bimg_len;
//     uint8		bimg_info;

//     /* Buffer holding the rmgr-specific data associated with this block */
//     bool		has_data;
//     char	   *data;
//     uint16		data_len;
//     uint16		data_bufsz;
// } DecodedBkpBlock;
// typedef struct DecodedXLogRecord
// {
//     /* Private member used for resource management. */
//     size_t		size;			/* total size of decoded record */
//     bool		oversized;		/* outside the regular decode buffer? */
//     struct DecodedXLogRecord *next; /* decoded record queue link */

//     /* Public members. */
//     XLogRecPtr	lsn;			/* location */
//     XLogRecPtr	next_lsn;		/* location of next record */
//     XLogRecord	header;			/* header */
//     RepOriginId record_origin;
//     TransactionId toplevel_xid; /* XID of top-level transaction */
//     char	   *main_data;		/* record's main data portion */
//     uint32		main_data_len;	/* main data portion's length */
//     int			max_block_id;	/* highest block_id in use (-1 if none) */
//     DecodedBkpBlock blocks[FLEXIBLE_ARRAY_MEMBER];
// } DecodedXLogRecord;

struct XLogReaderState
{
	/* ----------------------------------------
	 * Public parameters
	 * ----------------------------------------
	 */

	/*
	 * Data input callback (mandatory).
	 *
	 * This callback shall read at least reqLen valid bytes of the xlog page
	 * starting at targetPagePtr, and store them in readBuf.  The callback
	 * shall return the number of bytes read (never more than XLOG_BLCKSZ), or
	 * -1 on failure.  The callback shall sleep, if necessary, to wait for the
	 * requested bytes to become available.  The callback will not be invoked
	 * again for the same page unless more than the returned number of bytes
	 * are needed.
	 *
	 * targetRecPtr is the position of the WAL record we're reading.  Usually
	 * it is equal to targetPagePtr + reqLen, but sometimes xlogreader needs
	 * to read and verify the page or segment header, before it reads the
	 * actual WAL record it's interested in.  In that case, targetRecPtr can
	 * be used to determine which timeline to read the page from.
	 *
	 * The callback shall set *pageTLI to the TLI of the file the page was
	 * read from.  It is currently used only for error reporting purposes, to
	 * reconstruct the name of the WAL file where an error occurred.
	 */
	XLogPageReadCB read_page;

	/*
	 * System identifier of the xlog files we're about to read.  Set to zero
	 * (the default value) if unknown or unimportant.
	 */
	uint64		system_identifier;

	/*
	 * Opaque data for callbacks to use.  Not used by XLogReader.
	 */
	void	   *private_data;

	/*
	 * Start and end point of last record read.  EndRecPtr is also used as the
	 * position to read next, if XLogReadRecord receives an invalid recptr.
	 */
	XLogRecPtr	ReadRecPtr;		/* start of last record read */
	XLogRecPtr	EndRecPtr;		/* end+1 of last record read */

	/* ----------------------------------------
	 * private/internal state
	 * ----------------------------------------
	 */

	/* Buffer for currently read page (XLOG_BLCKSZ bytes) */
	char	   *readBuf;

	/* last read segment, segment offset, read length, TLI */
	XLogSegNo	readSegNo;
	uint32		readOff;
	uint32		readLen;
	TimeLineID	readPageTLI;

	/* beginning of last page read, and its TLI  */
	XLogRecPtr	latestPagePtr;
	TimeLineID	latestPageTLI;

	/* beginning of the WAL record being read. */
	XLogRecPtr	currRecPtr;
	/* timeline to read it from, 0 if a lookup is required */
	TimeLineID	currTLI;
	/*
	 * Safe point to read to in currTLI if current TLI is historical
	 * (tliSwitchPoint) or InvalidXLogRecPtr if on current timeline.
	 *
	 * Actually set to the start of the segment containing the timeline
	 * switch that ends currTLI's validity, not the LSN of the switch
	 * its self, since we can't assume the old segment will be present.
	 */
	XLogRecPtr	currTLIValidUntil;
	/*
	 * If currTLI is not the most recent known timeline, the next timeline to
	 * read from when currTLIValidUntil is reached.
	 */
	TimeLineID	nextTLI;

	/* Buffer for current ReadRecord result (expandable) */
	char	   *readRecordBuf;
	uint32		readRecordBufSize;

	/* Buffer to hold error message */
	char	   *errormsg_buf;

	/*
	 * Set at the end of recovery: the start point of a partial record at the
	 * end of WAL (InvalidXLogRecPtr if there wasn't one), and the start
	 * location of its first contrecord that went missing.
	 */
	XLogRecPtr	abortedRecPtr;
	XLogRecPtr	missingContrecPtr;
	/* Set when XLP_FIRST_IS_OVERWRITE_CONTRECORD is found */
	XLogRecPtr	overwrittenRecPtr;

    /* ----------------------------------------
	 * Decoded representation of current record
	 *
	 * Use XLogRecGet* functions to investigate the record; these fields
	 * should not be accessed directly.
	 * ----------------------------------------
	 * Start and end point of the last record read and decoded by
	 * XLogReadRecordInternal().  NextRecPtr is also used as the position to
	 * decode next.  Calling XLogBeginRead() sets NextRecPtr and EndRecPtr to
	 * the requested starting position.
	 */
    XLogRecPtr	DecodeRecPtr;	/* start of last record decoded */
    XLogRecPtr	NextRecPtr;		/* end+1 of last record decoded */
    XLogRecPtr	PrevRecPtr;		/* start of previous record decoded */

    /* Last record returned by XLogReadRecord(). */
    // DecodedXLogRecord *record;


    /*
	 * Buffer for decoded records.  This is a circular buffer, though
	 * individual records can't be split in the middle, so some space is often
	 * wasted at the end.  Oversized records that don't fit in this space are
	 * allocated separately.
	 */
    char	   *decode_buffer;
    size_t		decode_buffer_size;
    bool		free_decode_buffer; /* need to free? */
    char	   *decode_buffer_head; /* data is read from the head */
    char	   *decode_buffer_tail; /* new data is written at the tail */

    /*
     * Queue of records that have been decoded.  This is a linked list that
     * usually consists of consecutive records in decode_buffer, but may also
     * contain oversized records allocated with palloc().
     */
    // DecodedXLogRecord *decode_queue_head;	/* oldest decoded record */
    // DecodedXLogRecord *decode_queue_tail;	/* newest decoded record */
    uint32		segoff;
};

/* Get a new XLogReader */
extern XLogReaderState *XLogReaderAllocate(XLogPageReadCB pagereadfunc,
				   void *private_data);

/* Free an XLogReader */
extern void XLogReaderFree(XLogReaderState *state);

/* Read the next XLog record. Returns NULL on end-of-WAL or failure */
extern struct XLogRecord *XLogReadRecord(XLogReaderState *state,
			   XLogRecPtr recptr, char **errormsg);

/* Validate a page */
extern bool XLogReaderValidatePageHeader(XLogReaderState *state,
					XLogRecPtr recptr, char *phdr);

/* In GPDB, this is needed in the backend, too, for WAL replication tests. */
/* #ifdef FRONTEND */
#if 1
extern XLogRecPtr XLogFindNextRecord(XLogReaderState *state, XLogRecPtr RecPtr);
#endif   /* FRONTEND */

#endif   /* XLOGREADER_H */
