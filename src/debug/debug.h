#ifndef __debug_h_
#define __debug_h_

//shared library symbols
#ifndef DEBUGAPI
#  if defined(_WIN32) || defined(__CYGWIN__)
#    if defined(DEBUG_EXPORT_BUILD)
#      if defined(__GNUC__)
#        define DEBUGAPI __attribute__ ((dllexport)) extern
#      else
#        define DEBUGAPI __declspec(dllexport) extern
#      endif
#    else
#      if defined(__GNUC__)
#        define DEBUGAPI __attribute__ ((dllimport)) extern
#      else
#        define DEBUGAPI __declspec(dllimport) extern
#      endif
#    endif
#  elif defined(__GNUC__) && defined(DEBUG_EXPORT_BUILD)
#    define DEBUGAPI __attribute__ ((visibility ("default"))) extern
#  else
#    define DEBUGAPI extern
#  endif
#endif

//useless function to use as breakpoint with gdb ( >b debug )
DEBUGAPI void debug();

#endif //__debug_h_