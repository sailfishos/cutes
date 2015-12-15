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

#include <QMetaEnum>

#include <cutes/util.hpp>

namespace cutes { namespace js {

QVariantMap const & ObjectFactory::getMembers()
{
    return members_;
}

QJSValue ObjectFactory::create(QVariant const &v)
{
    auto args = v.toList();
    if (!args.size()) {
        qWarning() << "create: no params provided " << v;
        return QJSValue();
    }

    auto name = args[0];
    
    auto p = names_.find(name.toString());
    if (p == names_.end()) {
        qWarning() << "Can't find class " << name;
        return QJSValue();
    } else {
        auto params = args.size() > 1 ? args[1].toList() : QVariantList();
        return p->second(*engine_, params);
    }
}

void ObjectFactory::addEnums(QMetaObject const &mo, QString const &cls_name)
{
    QVariantMap cls_members;
    for (int i = 0; i < mo.enumeratorCount(); ++i) {
        auto emo = mo.enumerator(i);
        QVariantMap ids;
        for (int j = 0; j < emo.keyCount(); ++j)
            ids[emo.key(j)] = emo.value(j);

        cls_members[emo.name()] = ids;
    }
    members_[cls_name] = cls_members;
}

}}
