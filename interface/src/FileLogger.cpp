//
//  FileLogger.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 12/22/13.
//
//

#include "FileLogger.h"
#include "HifiSockAddr.h"
#include <FileUtils.h>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QDesktopServices>

const QString FILENAME_FORMAT = "hifi-log_%1_%2.txt";
const QString DATETIME_FORMAT = "yyyy-MM-dd_hh.mm.ss";
const QString LOGS_DIRECTORY = "Logs";

FileLogger::FileLogger() : _logData(NULL) {
    setExtraDebugging(false);

    _fileName = FileUtils::standardPath(LOGS_DIRECTORY);
    QHostAddress clientAddress = QHostAddress(getHostOrderLocalAddress());
    QDateTime now = QDateTime::currentDateTime();
    _fileName.append(QString(FILENAME_FORMAT).arg(clientAddress.toString(), now.toString(DATETIME_FORMAT)));
}

void FileLogger::addMessage(QString message) {
    QMutexLocker locker(&_mutex);
    emit logReceived(message);
    _logData.append(message);

    QFile file(_fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out <<  message;
    }
}

void FileLogger::locateLog() {
    FileUtils::locateFile(_fileName);
}
