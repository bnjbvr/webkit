# - Try to find WPE-mesa.
# Once done, this will define
#
#  WPE_MESA_FOUND - system has WPE-mesa.
#  WPE_MESA_INCLUDE_DIRS - the WPE-mesa include directories
#  WPE_MESA_LIBRARIES - link these to use WPE-mesa.
#
# Copyright (C) 2016 Igalia S.L.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_package(PkgConfig)
pkg_check_modules(PC_WPE_MESA QUIET wpe-mesa)

find_path(WPE_MESA_INCLUDE_DIRS
    NAMES wpe-mesa/view-backend-exportable-dma-buf.h
    HINTS ${PC_WPE_MESA_INCLUDEDIR} ${PC_WPE_MESA_INCLUDE_DIRS}
)

find_library(WPE_MESA_LIBRARIES
    NAMES WPEBackend-mesa
    HINTS ${PC_WPE_MESA_LIBDIR} ${PC_WPE_MESA_LIBRARY_DIRS}
)

mark_as_advanced(WPE_MESA_INCLUDE_DIRS WPE_MESA_LIBRARIES)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WPE_MESA REQUIRED_VARS WPE_MESA_INCLUDE_DIRS WPE_MESA_LIBRARIES)
