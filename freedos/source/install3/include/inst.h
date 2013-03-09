typedef struct {
  unsigned errors;
  unsigned warnings;
} inst_t;

inst_t
set_install (const char *diskset, char *fromdir, char *destdir);

inst_t
disk_install(const char *datfile, char *fromdir, char *destdir);
