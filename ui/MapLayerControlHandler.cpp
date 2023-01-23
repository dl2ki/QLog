#include <QFile>
#include <QSettings>

#include "MapLayerControlHandler.h"
#include "core/debug.h"

MODULE_IDENTIFICATION("qlog.ui.maplayercontrolhandler");

MapLayerControlHandler::MapLayerControlHandler(const QString configID,
                                               QObject *parent)
    : QObject(parent),
      configID(configID)
{
}

void MapLayerControlHandler::connectChannel(QWebEnginePage *page)
{
    FCT_IDENTIFICATION;

    QFile file(":/qtwebchannel/qwebchannel.js");

    if (!file.open(QIODevice::ReadOnly))
    {
        qCInfo(runtime) << "Cannot read qwebchannel.js";
        return;
    }

    QTextStream stream(&file);
    QString js;

    js.append(stream.readAll());
    js += " var webChannel = new QWebChannel(qt.webChannelTransport, function(channel) "
          "{ window.foo = channel.objects.layerControlHandler; });"
          " map.on('overlayadd', function(e){ "
          "  switch (e.name) "
          "  { "
          "     case 'Grid': "
          "        foo.handleLayerSelectionChanged('maidenheadConfWorked', 'on'); "
          "        break; "
          "     case 'Gray-Line': "
          "        foo.handleLayerSelectionChanged('grayline', 'on'); "
          "        break; "
          "     case 'Aurora': "
          "        foo.handleLayerSelectionChanged('auroraLayer', 'on'); "
          "        break; "
          "  } "
          "});"
          "map.on('overlayremove', function(e){ "
          "   switch (e.name) "
          "   { "
          "      case 'Grid': "
          "         foo.handleLayerSelectionChanged('maidenheadConfWorked', 'off'); "
          "         break; "
          "      case 'Gray-Line': "
          "         foo.handleLayerSelectionChanged('grayline', 'off'); "
          "         break; "
          "     case 'Aurora': "
          "        foo.handleLayerSelectionChanged('auroraLayer', 'off'); "
          "        break; "
          "   } "
          "});";
    page->runJavaScript(js);
}

void MapLayerControlHandler::restoreControls(QWebEnginePage *page)
{
    FCT_IDENTIFICATION;

    QSettings settings;
    QString js;

    settings.beginGroup(QString("%1/layerstate").arg(configID));

    QStringList keys = settings.allKeys();

    for ( const QString &key : qAsConst(keys))
    {
        qCDebug(runtime) << "key:" << key << "value:" << settings.value(key);

        if ( settings.value(key).toBool() )
        {
            js += QString("map.addLayer(%1);").arg(key);
        }
        else
        {
            js += QString("map.removeLayer(%1);").arg(key);
        }
    }
    qCDebug(runtime) << js;

    page->runJavaScript(js);

    connectChannel(page);
}

QString MapLayerControlHandler::injectMapMenuJS(bool gridLayer,
                                                bool grayline,
                                                bool aurora)
{
    FCT_IDENTIFICATION;
    QStringList options;

    if ( gridLayer )
    {
        options << "\"" + tr("Grid") + "\": maidenheadConfWorked";
    }

    if ( grayline )
    {
        options << "\"" + tr("Gray-Line") + "\": grayline";
    }

    if (aurora)
    {
        options << "\"" + tr("Aurora") + "\": auroraLayer";
    }

    QString ret = QString("var layerControl = new L.Control.Layers(null,"
                          "{ %1 },{}).addTo(map);").arg(options.join(","));

    qCDebug(runtime) << ret;

    return ret;
}

void MapLayerControlHandler::handleLayerSelectionChanged(const QVariant &data, const QVariant &state)
{
    FCT_IDENTIFICATION;

    qCDebug(function_parameters) << data << state;

    QSettings settings;

    settings.setValue(QString("%1/layerstate/%2").arg(configID,data.toString()),
                      (state.toString().toLower() == "on") ? true : false);
}
