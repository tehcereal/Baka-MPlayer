#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>
#include <QString>

class BakaEngine;

class UpdateManager : public QObject
{
    Q_OBJECT
public:
    explicit UpdateManager(QObject *parent = 0);
    ~UpdateManager();

    QMap<QString, QString> getInfo() { return info; }

public slots:
    bool CheckForUpdates();

//#if defined(Q_OS_WIN)
    bool DownloadUpdate(const QString &url, const QString &version);
    void ApplyUpdate(const QString &file, const QString &version);
//#endif

signals:
    void progressSignal(int percent);
    void messageSignal(QString msg);

private:
    BakaEngine *baka;

    QNetworkAccessManager *manager;
    QMap<QString, QString> info;
    bool busy;
};

#endif // UPDATEMANAGER_H
