# SPDX-License-Identifier: MIT
# Original Author: Serge Zaitsev
# Library: jsmn
# Version: 1.1.0
# Source: https://github.com/espressif/idf-extra-components/tree/master/jsmn
# Folder: .
# File: CMakeLists.txt
#
# MIT License
#
# Copyright (c) 2010 Serge Zaitsev
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

idf_component_register(INCLUDE_DIRS "include")

if(CONFIG_JSMN_PARENT_LINKS)
    target_compile_definitions(${COMPONENT_LIB} INTERFACE "-DJSMN_PARENT_LINKS")
endif()

if(CONFIG_JSMN_STRICT)
    target_compile_definitions(${COMPONENT_LIB} INTERFACE "-DJSMN_STRICT")
endif()

if(CONFIG_JSMN_STATIC)
    target_compile_definitions(${COMPONENT_LIB} INTERFACE "-DJSMN_STATIC")
endif()
