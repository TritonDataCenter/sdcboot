#ifndef Uncompression_H_
#define Uncompression_H_

/* Compression not currently supported! */
#define UncompressBuffer(InData, OutData, insize, outsize) \
      memcpy(OutData, InData, insize);

#endif
