#ifndef SUBDIR_FUNCTIONS_H_
#define SUBDIR_FUNCTIONS_H_

/* Order is important in order to fix errors correctly */

struct ScanDirFunction DirectoryChecks[] =
{
    {InitFirstClusterChecking,
     PreProcesFirstClusterChecking,
     CheckFirstCluster,
     PostProcesFirstClusterChecking,
     DestroyFirstClusterChecking
#ifdef ENABLE_LOGGING     
     , "first clusters"
#endif
    },
    
    {InitLFNChecking,
     PreProcesLFNChecking,
     CheckLFNInDirectory,
     PostProcesLFNChecking,
     DestroyLFNChecking
#ifdef ENABLE_LOGGING     
     , "Long file names"
#endif    
    }, 
    
    {InitDblNameChecking,
     PreProcesDblNameChecking,
     CheckDirectoryForDoubles,
     PostProcesDblNameChecking,
     DestroyDblNameChecking
#ifdef ENABLE_LOGGING    
     , "double names"
#endif    
    },
          
    {InitDblDotChecking,
     PreProcesDblDotChecking,
     CheckDirectoryForInvalidDblDots,
     PostProcesDblDotChecking,
     DestroyDblDotChecking
#ifdef ENABLE_LOGGING      
     , "double dots"
#endif    
    },
     
    {InitInvalidCharChecking,           
     PreProcesInvalidCharChecking,
     CheckEntryForInvalidChars,
     PostProcesInvalidCharChecking,
     DestroyInvalidCharChecking
#ifdef ENABLE_LOGGING    
     , "invalid chars"
#endif    
    }
};

#define AMOFNESTEDDIRCHECKS LENGTH(DirectoryChecks)

#endif