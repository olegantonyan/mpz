if(NOT DEFINED _DISABLE_INSTALLATION)
  # This variable is responsible for installation disabling.
  set(_DISABLE_INSTALLATION FALSE)
  # Replace install() with conditional installation.
  macro(install)
    if (NOT _DISABLE_INSTALLATION)
      _install(${ARGN})
    endif()
  endmacro()
endif()
