#ifndef TIME_LABEL_H_
#define TIME_LABEL_H_

struct TimeLabel {

    char** strings;         /* Strings to iterate over. */
    int    lastsec;         /* Last second interval.    */

    int    current;         /* Current string to display. */
    int    AmofStrings;     /* Amount of strings.         */

    int    ClearLength;     /* Length of the longest string. */
};

#endif
