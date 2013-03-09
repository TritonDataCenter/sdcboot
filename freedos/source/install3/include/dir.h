#ifdef __cplusplus
extern "C" {
#endif

#ifndef DIR_H
#define DIR_H

#ifdef unix
#define DIR_CHAR '/'
#else
#define DIR_CHAR '\\'
#define ALT_DIR_CHAR '/'
#endif

#define MAXPATH 260
#define MAXDIR 255

int mkdir(const char *path);
void fnmerge(char *path, const char *drive, const char *dir, const char *name, const char *ext);

#endif /* DIR_H */

#ifdef __cplusplus
}
#endif
