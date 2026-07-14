// zlib.c -- stock zlib 1.1.3 (general-purpose deflate/inflate compression by
// Jean-loup Gailly and Mark Adler). The Dreamcast build links zlib essentially
// unmodified, so this file is a landing spot for the upstream sources rather than
// a reconstruction: drop in the official zlib 1.1.3 distribution
//   adler32.c crc32.c compress.c uncompress.c deflate.c inflate.c trees.c
//   infblock.c inftrees.c infcodes.c infutil.c inffast.c gzio.c zutil.c
// (either concatenated here or added alongside), then wire them into the project.
//
// Only two things differ from upstream on the Dreamcast and belong to the port:
//   * zcalloc / zcfree route allocations through the Mnemo arena (MnemoAlloc /
//     MnemoFree) instead of malloc, so zlib shares the engine heap.
//   * the gz* temp files live under the DC save path (\CD-ROM\valve\SAVE).
// Those glue functions are the DC-specific part; everything else is upstream zlib.
