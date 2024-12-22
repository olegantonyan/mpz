/*
** libfault -- small library for crash diagnostics.
**
** Copyright (C) 2014 Austin Seipp.
** Copyright (c) 2010-2014 Phusion (inspired by Phusion Passenger)
** Copyright (c) 2009-2012, Salvatore Sanfilippo (register/stack dumping)
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in the
**     documentation and/or other materials provided with the distribution.
**   * Neither the name of Redis nor the names of its contributors may be used
**     to endorse or promote products derived from this software without
**     specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
**
** [ BSD3 license: http://www.opensource.org/licenses/mit-license.php ]
*/

#ifndef _LIBFAULTPP_H_
#define _LIBFAULTPP_H_

#include <type_traits>

namespace libfault
{

namespace // anon namespace to guard C APIs
{
#include <libfault.h>
}; /* namespace */

auto init(void) noexcept -> void                      { libfault_init(); }
auto install(void) noexcept -> void                   { libfault_install_handlers(); }

auto app_name(const char* s) noexcept -> void         { libfault_set_app_name(s); }
auto app_version(const char* s) noexcept -> void      { libfault_set_app_version(s); }
auto log_name(const char* s) noexcept -> void         { libfault_set_log_name(s); }
auto bugreport_url(const char* s) noexcept -> void    { libfault_set_bugreport_url(s); }

template <typename T, void F(T)>
auto diagnostics(T v) noexcept -> void {
  static_assert(std::is_pointer<T>::value, "Diagnostic callback data must be a valid pointer");

  libfault_set_custom_diagnostics([] (void* p) { F(static_cast<T>(p)); });
  libfault_set_custom_diagnostics_data(static_cast<void*>(v));
}

};

#endif /* _LIBFAULTPP_H_ */
