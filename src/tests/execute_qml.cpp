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

#include <QString>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QDebug>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        qDebug() << "Usage:" << (argc ? argv[0] : "app") << "<filename.qml>";
        ::exit(1);
    }
    QString script_file(argv[1]);
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine(script_file);
    return app.exec();
}

