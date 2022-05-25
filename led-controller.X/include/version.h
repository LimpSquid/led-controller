#ifndef VERSION_H
#define	VERSION_H

#ifdef __DEBUG
    // App or bootloader debug
    #define VERSION_MAJOR   255
    #define VERSION_MINOR   255
    #define VERSION_PATCH   255
#elif BOOTLOADER
    // Bootloader release
    #define VERSION_MAJOR   0
    #define VERSION_MINOR   0
    #define VERSION_PATCH   0
#else 
    // App release
    #define VERSION_MAJOR   0
    #define VERSION_MINOR   0
    #define VERSION_PATCH   2
#endif

#endif	/* VERSION_H */

