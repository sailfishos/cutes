/*
 * Copyright (C) 2013 Jolla Ltd. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#ifndef _CUTES_UTIL_HPP_
#define _CUTES_UTIL_HPP_

template <typename Enter, typename Exit>
struct Scope
{
    Scope(Enter enter_fn, Exit exit_fn)
        : finalize_(true)
        , exit_fn_(exit_fn)
    {
        enter_fn();
    }

    Scope(Scope &&src)
        : finalize_(true), exit_fn_(src.exit_fn_)
    {
        src.finalize_ = false;
    }
    ~Scope()
    {
        if (finalize_) exit_fn_();
    }
    bool finalize_;
    Exit exit_fn_;
private:
    Scope(Scope&);
};

template <typename Enter, typename Exit>
Scope<Enter, Exit> mk_scope(Enter enter_fn, Exit exit_fn)
{
    return Scope<Enter, Exit>(enter_fn, exit_fn);
}

template <typename ContainerT, typename FnT>
ContainerT filter(ContainerT const& src, FnT fn) {
    ContainerT res;
    for (auto &v: src) {
        if (fn(v))
            res.append(v);
    }
    return res;
}

#define STRINGIFY(x) #x
#define DQUOTESTR(x) STRINGIFY(x)

#endif // _CUTES_UTIL_HPP_
