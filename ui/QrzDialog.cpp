#include <QSettings>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QProgressDialog>
#include <QMessageBox>
#include <QSqlError>

#include "QrzDialog.h"
#include "ui_QrzDialog.h"
#include "core/debug.h"
#include "core/QRZ.h"
#include "models/SqlListModel.h"
#include "ui/LotwShowUploadDialog.h"
#include "logformat/AdiFormat.h"

MODULE_IDENTIFICATION("qlog.ui.qrzdialog");

QRZDialog::QRZDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QRZDialog)
{
    ui->setupUi(this);

    /* Upload */

    ui->myCallsignCombo->setModel(new SqlListModel("SELECT DISTINCT UPPER(station_callsign) FROM contacts ORDER BY station_callsign", ""));
    ui->myGridCombo->setModel(new SqlListModel("SELECT DISTINCT UPPER(my_gridsquare) FROM contacts WHERE station_callsign ='"
                                                + ui->myCallsignCombo->currentText()
                                                + "' ORDER BY my_gridsquare", ""));
    loadDialogState();
}

QRZDialog::~QRZDialog()
{
    delete ui;
}

void QRZDialog::upload()
{
    FCT_IDENTIFICATION;

    QByteArray data;
    QTextStream stream(&data, QIODevice::ReadWrite);

    AdiFormat adi(stream);
    QLocale locale;
    QString QSOList;
    int count = 0;

    /* https://www.qrz.com/docs/logbook/QRZLogbookAPI.html */
    /* ??? QRZ Support all ADIF Fields ??? */
    QString query_string = "SELECT * ";
    QString query_from   = "FROM contacts ";
    QString query_where =  "WHERE (qrzcom_qso_upload_status <> 'Y' OR qrzcom_qso_upload_status is NULL) ";
    QString query_order = " ORDER BY start_time ";

    saveDialogState();

    if ( !ui->myCallsignCombo->currentText().isEmpty() )
    {
        query_where.append(" AND station_callsign = '" + ui->myCallsignCombo->currentText() + "'");
    }

    if ( !ui->myGridCombo->currentText().isEmpty() )
    {
        query_where.append(" AND my_gridsquare = '" + ui->myGridCombo->currentText() + "'");
    }

    query_string = query_string + query_from + query_where + query_order;

    qCDebug(runtime) << query_string;

    QSqlQuery query(query_string);

    QList<QSqlRecord> qsos;

    while (query.next())
    {
        QSqlRecord record = query.record();

        QSOList.append(" "
                       + record.value("start_time").toDateTime().toTimeSpec(Qt::UTC).toString(locale.dateTimeFormat(QLocale::ShortFormat))
                       + " " + record.value("callsign").toString()
                       + " " + record.value("mode").toString()
                       + "\n");

        qsos.append(record);
    }

    count = qsos.count();

    if (count > 0)
    {
        /* TODO: rename it to ShowUploadDialog */
        LotwShowUploadDialog showDialog(QSOList);

        if ( showDialog.exec() == QDialog::Accepted )
        {
            QProgressDialog* dialog = new QProgressDialog(tr("Uploading to QRZ.com"),
                                                          tr("Cancel"),
                                                          0, count, this);
            dialog->setWindowModality(Qt::WindowModal);
            dialog->setValue(0);
            dialog->setAttribute(Qt::WA_DeleteOnClose, true);
            dialog->show();

            QRZ *qrz = new QRZ(dialog);

            connect(qrz, &QRZ::uploadedQSO, this, [qrz, dialog](int qsoID)
            {
                QString query_string = "UPDATE contacts "
                                       "SET qrzcom_qso_upload_status='Y', qrzcom_qso_upload_date = strftime('%Y-%m-%d',DATETIME('now', 'utc')) "
                                       "WHERE id = :qsoID";

                qCDebug(runtime) << query_string;

                QSqlQuery query_update;

                query_update.prepare(query_string);
                query_update.bindValue(":qsoID", qsoID);

                if ( ! query_update.exec() )
                {
                    qInfo() << "Cannot Update QRZCOM status for QSO number " << qsoID << " " << query_update.lastError().text();
                    qrz->cancelUploadContacts();
                }
                dialog->setValue(dialog->value() + 1);
            });

            connect(qrz, &QRZ::uploadFinished, this, [this, dialog, query_where, count](bool)
            {
                dialog->done(QDialog::Accepted);
                QMessageBox::information(this, tr("QLog Information"),
                                         tr("%n QSO(s) uploaded.", "", count));
            });

            connect(qrz, &QRZ::uploadError, this, [this, dialog](QString msg)
            {
                dialog->done(QDialog::Accepted);
                qCInfo(runtime) << "QRZ.com Upload Error: " << msg;
                QMessageBox::warning(this, tr("QLog Warning"),
                                     tr("Cannot upload the QSO(s): ") + msg);
            });

            qrz->uploadContacts(qsos);

            connect(dialog, &QProgressDialog::canceled, qrz, &QRZ::cancelUploadContacts);
        }
    }
    else
    {
        QMessageBox::information(this, tr("QLog Information"), tr("No QSOs found to upload."));
    }

}

void QRZDialog::uploadCallsignChanged(const QString &my_callsign)
{
    FCT_IDENTIFICATION;

    ui->myGridCombo->setModel(new SqlListModel("SELECT DISTINCT UPPER(my_gridsquare) FROM contacts WHERE station_callsign ='" + my_callsign + "' ORDER BY my_gridsquare", ""));

}

void QRZDialog::saveDialogState()
{
    FCT_IDENTIFICATION;

    QSettings settings;

    settings.setValue("qrzcom/last_mycallsign", ui->myCallsignCombo->currentText());
    settings.setValue("qrzcom/last_mygrid", ui->myGridCombo->currentText());
}

void QRZDialog::loadDialogState()
{
    FCT_IDENTIFICATION;

    QSettings settings;

    ui->myCallsignCombo->setCurrentText(settings.value("qrzcom/last_mycallsign").toString());
    ui->myGridCombo->setCurrentText(settings.value("qrzcom/last_mygrid").toString());
}