#include "icon_data.h"
#include <QByteArray>
#include <QImage>
#include <QFile>

namespace IconData {
    QPixmap getPixmap() {
        QFile file(":/icon/icon_base64.txt");
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = QByteArray::fromBase64(file.readAll());
            file.close();
            QImage image;
            image.loadFromData(data);
            return QPixmap::fromImage(image);
        }
        return QPixmap();
    }
}