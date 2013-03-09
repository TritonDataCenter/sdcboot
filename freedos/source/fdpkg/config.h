#ifndef __CONFIG_H
#define __CONFIG_H

/* Define the following to disable debug */
#define NDEBUG 1

/* Define the following to disable usage of the kitten library for localized messages */
/* #define NO_KITTEN 1 */

/* Define the following to enable complete backwards compatibility */
/* #define FEATURE_BACKCOMPAT 1 */

/* Define the following to allow a switch that outputs a SET command with slashes inverted */
#define FEATURE_ENV_SWITCHING 1

/* Define the following to enable support for URLs; to download them with wget or htget */
#define FEATURE_WGET 1

/* Define the following to enable scanning of own exe to act differently based on it's name */
/* #define FEATURE_BATCH_WRAPPERS 1 */

/* Define the following to enable support for 7-zip archives */
#define FEATURE_7ZIP 1

#endif

