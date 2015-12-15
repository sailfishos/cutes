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
#include "QmlAdapter.hpp"
#include "Actor.hpp"
#include "Env.hpp"
#include "util.hpp"

#include "qt_quick_types.hpp"

namespace cutes {

void setupDeclarative
(QCoreApplication &app, QQuickView &view, QString const &qml_path)
{
    QJSEngine *pengine = view.engine();
    if (!pengine)
        throw Error("View engine is null");
    auto script_env = new EnvImpl(pengine, app, *pengine);
    script_env->pushParentScriptPath(qml_path);

    registerDeclarativeTypes(DQUOTESTR(QML_NAMESPACE));
}

}
