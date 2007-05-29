!
!     Modifications:
!
!     kdb 951102 Add fciv_open, civerr declarations.
!     kdb 980515 Add execute_cls
!     pet 1999.07.28   Added operator EXTERNAL to prevent warning messages
!     pet 2004.10.18   Added FC_FSREAL8
!
      INTEGER*4 FC_CLOSE, FC_CREAT, FC_DUP, FC_EXECVP, FC_FCNTL, FC_GMTIME
      INTEGER*4 FC_LINK, FC_LSEEK, FC_MEMCPY, FC_PIPE, FC_READ, FC_SLEEP
      INTEGER*4 FC_SYSTEM, FC_TIME, FC_TM_G, FC_UNLINK, FC_WAIT, FC_WRITE
      INTEGER*4 FC_FLOCK_P, FC_OPEN, FC_FLOCK_G
      INTEGER*4 FC_UMASK
      INTEGER*4 PTR_CH,PTR_NC
      INTEGER*4 EXECUTE, NULL_TERM, FC_CONST_G, EXECUTE_D, EXECUTE_CLS
      INTEGER*4 FC_FREESP
      INTEGER*4 FC_PAUSE
      INTEGER*4 FC_CLOCK
      INTEGER*4 FC_FSIZE, FC_GETENV
      INTEGER*4 FC_TELL
      INTEGER*4 FC_GWINSZ, FC_GWINW, FC_CATCHSIGS, SYSERR
      INTEGER*4 FC_GETHOSTNAME, FC_NICE
      INTEGER*4 FCIV_OPEN, CIVERR
      REAL*8    FC_FSREAL8
!
      EXTERNAL  FC_CLOSE, FC_CREAT, FC_DUP, FC_EXECVP, FC_FCNTL, FC_GMTIME
      EXTERNAL  FC_LINK, FC_LSEEK, FC_MEMCPY, FC_PIPE, FC_READ, FC_SLEEP
      EXTERNAL  FC_SYSTEM, FC_TIME, FC_TM_G, FC_UNLINK, FC_WAIT, FC_WRITE
      EXTERNAL  FC_FLOCK_P, FC_OPEN, FC_FLOCK_G
      EXTERNAL  FC_UMASK
      EXTERNAL  PTR_CH, PTR_NC
      EXTERNAL  EXECUTE, NULL_TERM, FC_CONST_G, EXECUTE_D, EXECUTE_CLS
      EXTERNAL  FC_FREESP
      EXTERNAL  FC_PAUSE
      EXTERNAL  FC_CLOCK
      EXTERNAL  FC_FSIZE, FC_GETENV
      EXTERNAL  FC_TELL
      EXTERNAL  FC_GWINSZ, FC_GWINW, FC_CATCHSIGS, SYSERR
      EXTERNAL  FC_GETHOSTNAME, FC_NICE
      EXTERNAL  FCIV_OPEN, CIVERR
