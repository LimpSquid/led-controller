#ifndef VERSION_H
#define	VERSION_H

#ifdef __DEBUG
    // App or bootloader debug
    #define VERSION_MAJOR   0
    #define VERSION_MINOR   0
    #define VERSION_PATCH   0
#elif BOOTLOADER
    // Bootloader
    #define VERSION_MAJOR   1
    #define VERSION_MINOR   0
    #define VERSION_PATCH   2
#else
    // App
    #define VERSION_MAJOR   2
    #define VERSION_MINOR   0
    #define VERSION_PATCH   4
#endif

#endif	/* VERSION_H */

