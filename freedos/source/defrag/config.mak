#
# Configuration file, change this file to reflect your system
#

# defines:
caching  = -DUSE_SECTOR_CACHE -DWRITE_BACK_CACHE
media    = -DALLOW_REALDISK
image    = #-DDEBUG_IMAGEFILE
release  = -DNDEBUG       # uncomment when doing a release

# tools used:
compiler = tcc -c
options  = -O -d -Z -w -ml $(release)   # compiler options
libman   = tlib
linker   = tcc -ml -M
