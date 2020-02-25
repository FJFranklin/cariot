/* Copyright 2018-20 Francis James Franklin
 * 
 * Open Source under the MIT License - see LICENSE in the project's root folder
 */

#ifndef cariot_FIFO_hh
#define cariot_FIFO_hh

/** FIFO is a byte buffer where bytes are added and removed in first-in first-out order.
 */
template <typename T>
class FIFO {
private:
  T   buffer_start[FIFO_BUFSIZE]; ///< The buffer.
  T * buffer_end;                 ///< Pointer to the end of the buffer.
  T * data_start;                 ///< Pointer to the start of the data; if start == end, then no data.
  T * data_end;                   ///< Pointer to the end of the data; must point to a writable byte.

public:
  /** Empty the buffer for a fresh start.
   */
  inline void clear () {
    data_start = buffer_start;
    data_end   = buffer_start;
  }

  /** Returns true if the buffer is empty.
   */
  inline bool is_empty () {
    return (data_start == data_end);
  }

  /** Add a byte to the buffer; returns true if there was space.
   */
  inline bool push (T byte) {
    bool bCanPush = true;

    if (data_start < data_end) {
      if ((data_start == buffer_start) && (data_end + 1 == buffer_end)) {
        bCanPush = false;
      }
    } else if (data_start > data_end) {
      if (data_end + 1 == data_start) {
        bCanPush = false;
      }
    } // else (data_start == data_end) // buffer must be empty

    if (bCanPush) {
      *data_end = byte;

      if (++data_end == buffer_end) {
        data_end = buffer_start;
      }
    }
    return bCanPush;
  }

  /** Remove a byte from the buffer; returns true if the buffer wasn't empty.
   */
  inline bool pop (T & byte) {
    if (data_start == data_end) { // buffer must be empty
      return false;
    }
    byte = *data_start;

    if (++data_start == buffer_end) {
      data_start = buffer_start;
    }
    return true;
  }

  FIFO () :
    buffer_end(buffer_start+FIFO_BUFSIZE),
    data_start(buffer_start),
    data_end(buffer_start)
  {
    // ...
  }

  ~FIFO () {
    // ...
  }

  /** Read (and remove) multiple bytes from the buffer.
   * \param ptr    Pointer to an external byte array where the data should be written.
   * \param length Number of bytes to read from the buffer, if possible.
   * \return The number of bytes actually read from the buffer.
   */
  int read (T * ptr, int length) {
    int count = 0;

    if (ptr && length) {
      if (data_end > data_start) {
	count = data_end - data_start;             // i.e., bytes in FIFO
	count = (count > length) ? length : count; // or length, if less

	memcpy (ptr, data_start, count);
	data_start += count;

      } else if (data_end < data_start) {
	count = buffer_end - data_start;           // i.e., bytes in FIFO *at the end*
	count = (count > length) ? length : count; // or length, if less

	memcpy (ptr, data_start, count);
	data_start += count;

	if (data_start == buffer_end) { // wrap-around
	  data_start = buffer_start;

	  length -= count;                         // how much we still want to read

	  int extra = data_end - data_start;      // i.e., bytes in FIFO

	  if (length && extra) {                       // we can read more...
	    extra = (extra > length) ? length : extra; // or length, if less

	    memcpy (ptr, data_start, extra);
	    data_start += extra;

	    count += extra;
	  }
	}
      } // else (data_end == data_start) => FIFO is empty
    }
    return count;
  }

  /** Write multiple bytes to the buffer.
   * \param ptr    Pointer to an external byte array where the data should be read from.
   * \param length Number of bytes to write to the buffer, if possible.
   * \return The number of bytes actually written to the buffer.
   */
  int write (const T * ptr, int length) {
    int count = 0;

    if (ptr && length) {
      if (data_end > data_start) {
	/* this is where we need to worry about wrap-around
	 */
	if (data_start == buffer_start) { // we're *not* able to wrap-around
	  count = buffer_end - data_end - 1;         // i.e., usable free space in FIFO *at the end*
	  count = (count > length) ? length : count; // or length, if less

	  memcpy (data_end, ptr, count);
	  data_end += count;

	} else { // we *are* able to wrap-around
	  count = buffer_end - data_end;             // i.e., usable free space in FIFO *at the end*
	  count = (count > length) ? length : count; // or length, if less

	  memcpy (data_end, ptr, count);
	  data_end += count;

	  if (data_end == buffer_end) { // wrap-around
	    data_end = buffer_start;

	    length -= count;                             // how much we still want to write

	    int extra = data_start - data_end - 1;     // i.e., usable free space in FIFO

	    if (length && extra) {                       // we can write more...
	      extra = (extra > length) ? length : extra; // or length, if less

	      memcpy (data_end, ptr, extra);
	      data_end += extra;

	      count += extra;
	    }
	  }
	}
      } else if (data_end < data_start) {
	count = data_start - data_end - 1;         // i.e., usable free space in FIFO
	count = (count > length) ? length : count; // or length, if less

	/* don't need to worry about wrap-around
	 */
	memcpy (data_end, ptr, count);
	data_end += count;

      } else { // (data_end == data_start)
	/* the FIFO is empty - we can move the pointers for convenience
	 */
	data_start = buffer_start;
	data_end   = buffer_start;

	count = buffer_end - buffer_start - 1;     // i.e., maximum number of bytes the FIFO can hold
	count = (count > length) ? length : count; // or length, if less

	/* don't need to worry about wrap-around
	 */
	memcpy (data_end, ptr, count);
	data_end += count;
      }
    }
    return count;
  }

  int available() const {
    int count = 0;

    if (data_end > data_start) {
      count = data_end - data_start;         // i.e., bytes in FIFO
    } else if (data_end < data_start) {
      count  = buffer_end - data_start;      // i.e., bytes in FIFO *at the end*
      count += data_end - buffer_start;      // i.e., bytes in FIFO
    } // else (data_end == data_start) => FIFO is empty

    return count;
  }

  int availableToWrite() const {
    int count = 0;

    if (data_end > data_start) {
      /* this is where we need to worry about wrap-around
       */
      if (data_start == buffer_start) { // we're *not* able to wrap-around
	count = buffer_end - data_end - 1;       // i.e., usable free space in FIFO *at the end*
      } else { // we *are* able to wrap-around
	count  = buffer_end - data_end;          // i.e., usable free space in FIFO *at the end*
	count += data_start - buffer_start - 1;  // i.e., usable free space in FIFO
      }
    } else if (data_end < data_start) {
      count = data_start - data_end - 1;         // i.e., usable free space in FIFO
    } else { // (data_end == data_start)
      count = buffer_end - buffer_start - 1;     // i.e., maximum number of bytes the FIFO can hold
    }
    return count;
  }
};

#endif /* !cariot_FIFO_hh */
