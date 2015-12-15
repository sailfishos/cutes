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

#include "EngineAccess.hpp"
#include "QsActor.hpp"
#include "QsEnv.hpp"
#include "QmlAdapter.hpp"

#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeExpression>
#include <QtDeclarative>
#include <QtDeclarative/QDeclarativeExtensionPlugin>

namespace QsExecute {

class Plugin : public QDeclarativeExtensionPlugin
{
public:
    void initializeEngine(QDeclarativeEngine *engine, const char *)
    {
        QJSEngine *script_engine = getDeclarativeScriptEngine(*engine->rootContext());
        loadEnv(*QCoreApplication::instance(), *script_engine);
    }

    void registerTypes(char const *uri)
    {
        registerDeclarativeTypes(uri);
    }
};

}

Q_EXPORT_PLUGIN2(QML_NAMESPACE, QsExecute::Plugin);
