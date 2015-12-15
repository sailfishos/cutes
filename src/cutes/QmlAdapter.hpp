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

#ifndef _QTSCRIPT_QML_ADAPTER_H_
#define _QTSCRIPT_QML_ADAPTER_H_

#include "Env.hpp"

#include <QCoreApplication>
#include "qt_quick_types.hpp"

namespace cutes {

#define QML_NAMESPACE Mer.Cutes
static inline void registerDeclarativeTypes(char const *uri)
{
    qmlRegisterType<QmlActor>(uri, 1, 1, "CutesActor");
    qmlRegisterType<Adapter>(uri, 1, 1, "CutesAdapter");
}

void setupDeclarative(QCoreApplication &, QDeclarativeView &, QString const &);
QJSEngine *getDeclarativeScriptEngine(QDeclarativeContext &);
QJSEngine *getDeclarativeScriptEngine(QDeclarativeEngine *decl_eng);

}

#endif // _QTSCRIPT_QML_ADAPTER_H_
