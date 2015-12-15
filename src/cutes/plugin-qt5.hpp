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

#ifndef _CUTES_QML_PLUGIN_QT5_HPP_
#define _CUTES_QML_PLUGIN_QT5_HPP_

#include "util.hpp"

#include "QmlAdapter.hpp"

#include <QtQml/QQmlExtensionPlugin>
#include "qt_quick_types.hpp"

namespace cutes
{

class Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DQUOTESTR(QML_NAMESPACE))
public:
    void initializeEngine(QQmlEngine *engine, const char *)
    {
        auto app = QCoreApplication::instance();
        if (app)
            new EnvImpl(app, *app, *engine);
    }

    void registerTypes(char const* uri) {
        registerDeclarativeTypes(uri);
    }
};

}

#endif // _CUTES_QML_PLUGIN_QT5_HPP_
