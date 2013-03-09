
#
# Configuration file, change this file to reflect your system
#

# tools used:
compiler = tcc -c
options  = -O -d -Z -w -ml   # compiler options
libman   = tlib
linker   = tcc -ml -M

# defines:
caching        = -DUSE_SECTOR_CACHE
media          = -DALLOW_REALDISK
memory_manager = 
logging        = 
#logging        = -DENABLE_LOGGING
