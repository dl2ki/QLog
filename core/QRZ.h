#ifndef QRZ_H
#define QRZ_H

#include <QObject>
#include <QString>
#include <QSqlRecord>
#include "core/GenericCallbook.h"

class QNetworkAccessManager;
class QNetworkReply;

class QRZ : public GenericCallbook
{
    Q_OBJECT

public:
    explicit QRZ(QObject *parent = nullptr);
    ~QRZ();

    QNetworkReply* uploadContact(const QSqlRecord record);
    void uploadContacts(const QList<QSqlRecord>);

    const static QString SECURE_STORAGE_KEY;
    const static QString SECURE_STORAGE_API_KEY;
    const static QString CONFIG_USERNAME_KEY;
    const static QString CONFIG_USERNAME_API_KEY;
    const static QString CONFIG_USERNAME_API_CONST;
    const static QString CALLBOOK_NAME;

signals:
    void uploadFinished(bool result);
    void uploadedQSO(int);
    void uploadError(QString);

public slots:
    void queryCallsign(QString callsign) override;
    void cancelUploadContacts();

private slots:
    void processReply(QNetworkReply* reply);

private:
    QNetworkAccessManager* nam;
    QString sessionId;
    QString queuedCallsign;
    bool incorrectLogin;
    QString lastSeenPassword;
    QList<QSqlRecord> queuedContacts4Upload;
    bool cancelUpload;
    QNetworkReply *lastUploadReply;

    void authenticate();
    QNetworkReply *actionInsert(QByteArray& data, QString insertPolicy);
    QMap<QString, QString> parseActionResponse(const QString);
};

#endif // QRZ_H
