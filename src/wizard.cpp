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

#include <QDesktopServices>
#include <QSystemTrayIcon>
#include <QApplication>
#include <QStyle>
#include <QMenu>
#include <QUrl>

#include "qtsingleapplication/QtSingleApplication"
#include "editsolutiondialog.h"
#include "editprefixdialog.h"
#include "selectarchdialog.h"
#include "downloaddialog.h"
#include "solutiondialog.h"
#include "settingsdialog.h"
#include "outputdialog.h"
#include "scriptdialog.h"
#include "aboutdialog.h"
#include "filesystem.h"
#include "mainmenu.h"
#include "executor.h"
#include "dialogs.h"
#include "wizard.h"

const QString PREPARE_PACKAGES = "ww_installed_%1()\n{\n%2\n}\nww_install_%1()\n{\n%3\n}\n";
const QString PREPARE_FUNCTIONS = "ww_%1()\n{\n%2\n}\n";
const QString VERSION_ERR = QObject::tr("Please install a newer version of Wine Wizard.\n\nThe current version is %1.\n" \
                                        "The required version is %2.\n\nWine Wizard will exit.");

Wizard::Wizard(bool trayVisible, bool autoclose, QObject *parent) :
    QObject(parent),
    mTray(trayVisible ? new QSystemTrayIcon(QIcon::fromTheme("winewizard"), this) : nullptr)
{
    QFile f(FS::config().absoluteFilePath("style.qss"));
    if (f.exists())
    {
        f.open(QFile::ReadOnly);
        qApp->setStyleSheet(f.readAll());
    }
    if (mTray)
    {
        mTray->setProperty("Autoclose", autoclose);
        connect(mTray, &QSystemTrayIcon::activated, this, [this]{ if (!SingletonWidget::exists()) showMenu(); });
        mTray->show();
    }
}

void Wizard::start(const QString &cmdLine)
{
    if (!SingletonWidget::exists())
    {
        if (cmdLine.isEmpty())
        {
            if ((mTray && mTray->property("Autoclose").toBool()) || !mTray)
                showMenu();
        }
        else if (mBusyList.isEmpty())
            install(cmdLine);
        else
            Dialogs::error(tr("Another installation is already running!"));
    }
}

void Wizard::showMenu()
{
    MainMenu menu(mTray ? mTray->property("Autoclose").toBool() : true, mRunList, mBusyList);
    QAction *act = menu.exec();
    if (!act)
        return;
    switch (act->data().toInt())
    {
    case MainMenu::Install:
        {
            QString exe = Dialogs::open(tr("Select Installer"), tr("Executable files (*.exe *.msi)"));
            if (!exe.isEmpty())
                install(exe + '\n' + QFileInfo(exe).absolutePath() + '\n');
        }
        break;
    case MainMenu::Debug:
        {
            QString exe = act->property("Exe").toString();
            QString args = act->property("Args").toString();
            QString workDir = act->property("WorkDir").toString();
            QString prefixHash = act->property("PrefixHash").toString();
            QString shortcut = act->property("Shortcut").toString();
            mRunList.append(prefixHash);
            QString script = FS::readFile(":/run").arg(exe).arg(workDir).arg(args);
            Ex::Out out = Ex::debug(script, prefixHash);
            mRunList.removeOne(prefixHash);
            if (qApp->property("Quit").toBool())
                return;
            if (Dialogs::finish(prefixHash))
            {
                QSettings s(shortcut, QSettings::IniFormat);
                s.setValue("Debug", false);
            }
            else
                OutputDialog(out).exec();
        }
        break;
    case MainMenu::Run:
        {
            QString exe = act->property("Exe").toString();
            QString args = act->property("Args").toString();
            QString workDir = act->property("WorkDir").toString();
            QString prefixHash = act->property("PrefixHash").toString();
            mRunList.append(prefixHash);
            Ex::release(FS::readFile(":/run").arg(exe).arg(workDir).arg(args), prefixHash);
            mRunList.removeOne(prefixHash);
        }
        break;
    case MainMenu::RunFile:
        {
            QString prefixHash = act->property("PrefixHash").toString();
            QString dir = FS::drive(prefixHash).absolutePath();
            QString exe = Dialogs::open(tr("Select Installer"), tr("Executable files (*.exe *.msi)"), nullptr, dir);
            if (!exe.isEmpty())
            {
                mRunList.append(prefixHash);
                QString script = FS::readFile(":/run").arg(exe).arg(QString()).arg(QString());
                Ex::Out out = Ex::debug(script, prefixHash);
                mRunList.removeOne(prefixHash);
                if (qApp->property("Quit").toBool())
                    return;
                if (!Dialogs::finish(prefixHash))
                    OutputDialog(out).exec();
            }
        }
        break;
    case MainMenu::Edit:
        EditPrefixDialog(act->property("PrefixHash").toString()).exec();
        break;
    case MainMenu::Terminate:
        {
            QString solutinName = act->property("PrefixName").toString();
            QString prefixHash = act->property("PrefixHash").toString();
            if (Dialogs::confirm(tr(R"(Are you sure you want to terminate "%1"?)").arg(solutinName)))
                Ex::wait(FS::readFile(":/terminate"), prefixHash);
        }
        break;
    case MainMenu::Browse:
        {
            QString prefixHash = act->property("PrefixHash").toString();
            FS::browse(FS::prefix(prefixHash).absolutePath());
        }
        break;
    case MainMenu::Delete:
        {
            QString prefixName = act->property("PrefixName").toString();
            QString prefixHash = act->property("PrefixHash").toString();
            if (Dialogs::confirm(tr(R"(Are you sure you want to delete "%1"?)").arg(prefixName)))
                FS::removePrefix(prefixHash);
        }
        break;
    case MainMenu::Settings:
        SettingsDialog().exec();
        break;
    case MainMenu::About:
        AboutDialog().exec();
        break;
    case MainMenu::Help:
        QDesktopServices::openUrl(QUrl(HELP_URL));
        break;
    case MainMenu::Quit:
        if (Dialogs::confirm(tr("Are you sure you want to quit from Wine Wizard?")))
        {
            QString termScript = FS::readFile(":/terminate");
            for (const QString &prefixHash : mRunList)
                Ex::wait(QString(termScript), prefixHash);
            qApp->setProperty("Quit", true);
            QApplication::exit();
        }
        break;
    }
}

void Wizard::install(const QString &cmdLine)
{
    QStringList cmdList = cmdLine.split('\n', QString::SkipEmptyParts);
    QString exe = cmdList.takeFirst();
    QString workDir = cmdList.takeFirst();
    QString args = cmdList.join(' ');
    if (!QFile::exists(exe))
    {
        Dialogs::error(tr(R"(File "%1" not found!)").arg(exe));
        return;
    }
    if (!testSuffix(exe))
    {
        Dialogs::error(tr(R"(File "%1" is not a valid Windows application!)").arg(exe));
        return;
    }
    QString prefixName, arch, bs, acs, as;
    if (prepare(prefixName, arch, bs, acs, as))
    {
        QString prefixHash = FS::hash(prefixName);
        if (FS::prefix(prefixHash).exists())
            FS::removePrefix(prefixHash);
        mBusyList.append(prefixHash);
        Ex::terminal(bs, prefixHash);
        QSettings sol(FS::prefix(prefixHash).absoluteFilePath(".settings"), QSettings::IniFormat);
        sol.setIniCodec("UTF-8");
        sol.setValue("Name", prefixName);
        mRunList.append(prefixHash);
        Ex::release(FS::readFile(":/create"), prefixHash);
        mRunList.removeOne(prefixHash);
        if (qApp->property("Quit").toBool())
            return;
        QString wmbPath = FS::sys32(prefixHash, arch).absoluteFilePath("winemenubuilder.exe");
        QFile::remove(wmbPath);
        QFile::copy(":/winemenubuilder.exe", wmbPath);
        if (arch == "64")
        {
            wmbPath = FS::sys64(prefixHash).absoluteFilePath("winemenubuilder.exe");
            QFile::remove(wmbPath);
            QFile::copy(":/winemenubuilder64.exe", wmbPath);
        }
        if (!acs.isEmpty())
            Ex::terminal(acs, prefixHash);
        QString script = FS::readFile(":/run").arg(exe).arg(workDir).arg(args);
        mRunList.append(prefixHash);
        Ex::Out out = Ex::debug(script, prefixHash);
        mRunList.removeOne(prefixHash);
        if (qApp->property("Quit").toBool())
            return;
        Ex::terminal(as, prefixHash);
        mBusyList.removeOne(prefixHash);
        if (!Dialogs::finish(prefixHash))
            OutputDialog(out).exec();
    }
}

bool Wizard::testSuffix(const QFileInfo &path) const
{
    QString suffix = path.suffix().toUpper();
    return suffix == "EXE" || suffix == "MSI";
}

bool Wizard::prepare(QString &name, QString &arch, QString &bs, QString &acs, QString &as) const
{
    QDir cache = FS::cache();
    QString repoPath = cache.absoluteFilePath("main.wwrepo");
    {
        DownloadDialog dd(QStringList(REPO_URL), repoPath);
        if (dd.exec() != QDialog::Accepted)
            return false;
    }
    QSettings r(repoPath, QSettings::IniFormat);
    r.setIniCodec("UTF-8");
    QString repoVer = r.value("WineWizardVersion").toString();
    if (repoVer.isEmpty())
    {
        Dialogs::error(tr("Incorrect repository file format!"));
        return false;
    }
    QString appVer = QApplication::applicationVersion();
    if (appVer != repoVer)
    {
        Dialogs::error(VERSION_ERR.arg(appVer).arg(repoVer));
        QDesktopServices::openUrl(QUrl(DOWNLOAD_URL));
        return false;
    }
    clearRepository();
    SolutionDialog solDlg(mRunList);
    if (solDlg.exec() != QDialog::Accepted)
        return false;
    SelectArchDialog sad;
    if (sad.exec() != QDialog::Accepted)
        return false;
    arch = sad.arch();
    QString url = API_URL + "?c=get&slug=" + solDlg.slug() + "&arch=" + arch;
    QString outFile = FS::temp().absoluteFilePath("solution");
    {
        DownloadDialog dd(QStringList(url), outFile);
        if (dd.exec() != QDialog::Accepted)
            return false;
    }
    QByteArray data = FS::readFile(FS::temp().absoluteFilePath("solution")).toUtf8();
    QJsonParseError err;
    QJsonDocument jd = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError)
    {
        Dialogs::error(tr("Incorrect solution file format!"));
        return false;
    }
    QJsonObject jo = jd.object();
    name = jo.value("name").toString();
    QString bw = jo.value("bw").toString();
    QString aw = jo.value("aw").toString();
    QStringList bp;
    QJsonArray bpArr = jo.value("bp").toArray();
    for(QJsonArray::const_iterator iter = bpArr.begin(); iter != bpArr.end(); ++iter)
        bp.append((*iter).toString());
    QStringList ap;
    QJsonArray apArr = jo.value("ap").toArray();
    for(QJsonArray::const_iterator iter = apArr.begin(); iter != apArr.end(); ++iter)
        ap.append((*iter).toString());
    QString bScript = jo.value("bs").toString();
    QString aScript = jo.value("as").toString();
    QSet<QString> files;
    r.beginGroup("Packages" + arch);
    required(bw, files, &r);
    required(aw, files, &r);
    for (const QString &p : bp)
        required(p, files, &r);
    for (const QString &p : ap)
        required(p, files, &r);
    r.endGroup();
    r.beginGroup("Files");
    for (const QString &f : files)
    {
        r.beginGroup(f);
        QString checksum = r.value("Sum").toString();
        QString out = FS::cache().absoluteFilePath(f);
        if (!QFile::exists(out) || !FS::checkFileSum(out, checksum))
        {
            QStringList mirrors = r.value("Mirrors").toStringList();
            QStringList reList = r.value("RE").toStringList();
            DownloadDialog dd(mirrors, out, nullptr, checksum, reList);
            if (dd.exec() != QDialog::Accepted)
                return false;
        }
        r.endGroup();
    }
    r.endGroup();
    QString constScript = makeConstScript(arch);
    bs = constScript;
    QSettings s("winewizard", "settings");
    s.beginGroup("VideoSettings");
    QString scrW = s.value("ScreenWidth").toString();
    QString scrH = s.value("ScreenHeight").toString();
    QString vmSize = s.value("VideoMemorySize").toString();
    s.endGroup();
    QString is = r.value("Init").toString() + '\n';
    bs += QString(is).arg(arch).arg(bw).arg(scrW + 'x' + scrH).arg(vmSize) + '\n';
    if (!bp.isEmpty() || !bScript.isEmpty())
    {
        acs = bs;
        for (const QString &p : bp)
            acs += "ww_install " + p + '\n';
    }
    bs += "ww_install_wine " + QString(bw) + '\n';
    as = constScript;
    as += QString(is).arg(arch).arg(aw).arg(scrW + 'x' + scrH).arg(vmSize);
    if (aw != bw)
        as += "ww_install_wine " + QString(aw) + '\n';
    for (const QString &p : ap)
        as += "ww_install " + p + '\n';
    if ((!bScript.isEmpty() || !aScript.isEmpty()) && s.value("UseScripts", false).toBool())
        if (ScriptDialog(bScript, aScript).exec() == QDialog::Accepted)
        {
            if (!bScript.isEmpty())
                acs += "ww_info 'Start additional script ...'\n" + bScript.replace("\\", "\\\\") + '\n';
            if (!aScript.isEmpty())
                as += "ww_info 'Start additional script ...'\n" + aScript.replace("\\", "\\\\") + '\n';
        }
    as += r.value("Done").toString();
    return true;
}

void Wizard::required(const QString &package, QSet<QString> &res, QSettings *r) const
{
    r->beginGroup(package);
    for (const QString &f : r->value("Files").toStringList())
        res.insert(f);
    QStringList req = r->value("Required").toStringList();
    r->endGroup();
    for (const QString &p : req)
        required(p, res, r);
}

void Wizard::clearRepository() const
{
    QDir cache = FS::cache();
    QSettings r(cache.absoluteFilePath("main.wwrepo"), QSettings::IniFormat);
    r.beginGroup("Files");
    QStringList allFiles = r.childGroups();
    r.endGroup();
    allFiles.append("main.wwrepo");
    for (const QFileInfo &f : cache.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden))
        if (!allFiles.contains(f.fileName()))
        {
            if (f.isDir())
                QDir(f.absoluteFilePath()).removeRecursively();
            else
                QFile::remove(f.absoluteFilePath());
        }
}

QString Wizard::makeConstScript(const QString &arch) const
{
    QString res;
    QSettings r(FS::cache().absoluteFilePath("main.wwrepo"), QSettings::IniFormat);
    r.beginGroup("Functions");
    for (const QString &f : r.childGroups())
    {
        r.beginGroup(f);
        QString body = r.value("Body").toString();
        res += PREPARE_FUNCTIONS.arg(f).arg(body);
        r.endGroup();
    }
    r.endGroup();
    r.beginGroup("Packages" + arch);
    for (const QString &p : r.childGroups())
    {
        r.beginGroup(p);
        int type = r.value("Type", PT_PACKAGE).toInt();
        if (type == PT_PACKAGE)
        {
            QString check = r.value("Check").toString();
            QString install = r.value("Install").toString();
            res += PREPARE_PACKAGES.arg(p).arg(check).arg(install);
        }
        r.endGroup();
    }
    r.endGroup();
    return res;
}
