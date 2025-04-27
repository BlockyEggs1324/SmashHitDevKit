#include "SegmentLoader.h"
#include "qxmlstream.h"

std::array<GLfloat, 3> loadColour(QString rootDir, QString tempName) {
    QFile file(rootDir + "/templates.xml");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Error", "Failed to open template file");
        return {1.f, 1.f, 1.f};
    }

    QXmlStreamReader xml(&file);
    std::array<GLfloat, 3> colour;

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name().toString() == "template") {
            if (xml.attributes().value("name") == tempName) {
                // Inside the correct <template> tag
                while (!(xml.isEndElement() && xml.name().toString() == "template")) {
                    xml.readNext();

                    if (xml.isStartElement() && xml.name().toString() == "properties") {
                        QString colorAttr = xml.attributes().value("color").toString();
                        QStringList values = colorAttr.split(" ");

                        if (values.size() == 3) {
                            colour[0] = values[0].toFloat();
                            colour[1] = values[1].toFloat();
                            colour[2] = values[2].toFloat();
                        }
                        break;  // Found color, exit inner loop
                    }
                }
                break;  // Found the template, exit main loop
            }
        }
    }

    if (xml.hasError()) {
        QMessageBox::warning(nullptr, "Error", "Error parsing templates.xml: " + xml.errorString());
    }

    file.close();
    return colour;
}

Segment Loader::loadLevelSegment(const QString& rootDir, const QString& filename, const bool useRootDir) {
    QString path;

    if (useRootDir) path = rootDir + filename;
    else path = filename;

    // Open the XML file
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Error", "Failed to open file: " + path);
        return Segment();
    }

    QXmlStreamReader xml(&file);
    Segment segment;

    // Find last '/' or '\\'
    size_t lastSlash = filename.toStdString().find_last_of("/\\");
    std::string name = (lastSlash == std::string::npos) ? filename.toStdString() : filename.toStdString().substr(lastSlash + 1);

    // Remove ".xml" suffix if it exists
    if (name.size() >= 4 && name.substr(name.size() - 4) == ".xml") {
        name = name.substr(0, name.size() - 4);
    }
    segment.name = QString::fromStdString(name);

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const auto name = xml.name().toString();

            if (name == "segment") {
                QStringList sizeParts = xml.attributes().value("size").toString().split(' ');
                if (sizeParts.size() == 3)
                    segment.size = QVector3D(sizeParts[0].toFloat(), sizeParts[1].toFloat(), sizeParts[2].toFloat());

                segment.templateType = xml.attributes().value("template").toString();
            }

            else if (name == "box") {
                Box box;

                QStringList sizeParts = xml.attributes().value("size").toString().split(' ');
                QStringList posParts = xml.attributes().value("pos").toString().split(' ');

                if (sizeParts.size() == 3)
                    box.size = QVector3D(sizeParts[0].toFloat(), sizeParts[1].toFloat(), sizeParts[2].toFloat());

                if (posParts.size() == 3)
                    box.pos = QVector3D(posParts[0].toFloat(), posParts[1].toFloat(), posParts[2].toFloat());

                box.hidden = xml.attributes().value("hidden").toInt();
                box.templateType = xml.attributes().value("template").toString();
                box.colour = loadColour(rootDir, box.templateType);

                segment.boxes.push_back(box);
            }

            else if (name == "obstacle") {
                Obstacle obs;

                QStringList posParts = xml.attributes().value("pos").toString().split(' ');
                if (posParts.size() == 3)
                    obs.pos = QVector3D(posParts[0].toFloat(), posParts[1].toFloat(), posParts[2].toFloat());

                obs.hidden = xml.attributes().value("hidden").toInt();
                obs.type = xml.attributes().value("type").toString();
                obs.templateType = xml.attributes().value("template").toString();
                obs.mode = xml.attributes().hasAttribute("mode") ? xml.attributes().value("mode").toInt() : 0;

                segment.obstacles.push_back(obs);
            }
        }
    }

    return segment;
}

std::vector<Rect3D> Loader::getRects(const std::vector<Box>& boxes) {
    std::vector<Rect3D> rects;
    for (const Box& box : boxes) {
        Rect3D newRect(box.pos, box.size);
        newRect.setColour(box.colour);
        rects.push_back(newRect);
    }
    return rects;
}
