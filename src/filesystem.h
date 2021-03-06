/***************************************************************************
 *   Copyright (C) 2016 by Vitalii Kachemtsev <LLIAKAJL@yandex.ru>         *
 *                                                                         *
 *   This file is part of Wine Wizard.                                     *
 *                                                                         *
 *   Wine Wizard is free software: you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Wine Wizard is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with Wine Wizard.  If not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QDir>

namespace FS
{
    QDir cache();
    QDir data();
    QDir config();
    QDir temp();

    QDir prefix(const QString &prefixHash);
    QDir devices(const QString &prefixHash);
    QDir drive(const QString &prefixHash, const QString &letter = "c:");
    QDir driveTarget(const QString &prefixHash, const QString &letter = "c:");
    QDir icons(const QString &prefixHash);
    QDir shortcuts(const QString &prefixHash);
    QDir links(const QString &prefixHash);
    QDir documents(const QString &prefixHash);
    QDir wine(const QString &prefixHash);
    QDir packages(const QString &prefixHash);
    QDir windows(const QString &prefixHash);
    QDir sys32(const QString &prefixHash, const QString &arch = "32");
    QDir sys64(const QString &prefixHash);
    QDir users(const QString &prefixHash);
    QDir user(const QString &prefixHash);

    QString readFile(const QString &filePath);
    void browse(const QString &path);
    QString hash(const QString &str);
    bool checkFileSum(const QString &filePath, const QString &checksum);

    void removePrefix(const QString &prefixHash, QWidget *parent = nullptr);
    QString toWinPath(const QString &prefixHash, const QString &path);
    QString toUnixPath(const QString &prefixHash, const QString &path);
}

#endif // FILESYSTEM_H
