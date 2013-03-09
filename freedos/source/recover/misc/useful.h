/* USEFUL.H

    some useful macros
*/
                
        /* LENGTH(array) returns the ELEMENT count of an array */
#define LENGTH(array) (sizeof(array) / sizeof(array[0]))    

        /* UNUSED(x) avaoid warnings, that an argument is not used */
#if defined( __TURBOC__) || defined(__GNUC__)
    #define UNUSED(x) if (x);       
#elif defined(_MSC_VER)
    #define UNUSED(x) x;
#else 
    #define UNUSED(x) x;
#endif  

              /* this is used for local functions.
                 define as "" if you want to see it in your map,
                 define as "static" else
              */   
#define STATIC                     
