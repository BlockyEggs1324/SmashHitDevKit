#ifndef TEXTUREEXTRACTOR_H
#define TEXTUREEXTRACTOR_H

#include <QBuffer>
#include <QImage>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QDebug>
#include <QCoreApplication>
#include <QtZlib/QtZlibDepends>
#include <zlib.h>
#include <vector>

// Constants
constexpr int MAX_IMAGE_BOUNDS = 4096;
constexpr qint64 MAX_INPUT_FILE_SIZE = 1LL << 30; // 1 GiB
constexpr int PVRTC2_HEADER_SIZE = 64;

struct HeaderV0V1 {
    quint32 magic;
    quint32 lengthFirst;
    quint32 lengthSecond;
};

bool readHeaderV0V1(QFile &file, HeaderV0V1 &header) {
    if (!file.isOpen()) return false;

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in >> header.magic >> header.lengthFirst >> header.lengthSecond;

    return !in.atEnd();
}

// Decompress zlib-compressed data using QZlib-style approach
QByteArray decompressZlibData(const QByteArray &compressedData) {
    if (compressedData.isEmpty()) return {};

    // Estimate decompressed size (you can make this larger if needed)
    uLongf decompressedSize = 4 * 1024 * 1024; // 4 MB initially
    QByteArray decompressedData;
    decompressedData.resize(decompressedSize);

    int result = uncompress(reinterpret_cast<Bytef*>(decompressedData.data()), &decompressedSize,
                            reinterpret_cast<const Bytef*>(compressedData.constData()), compressedData.size());

    if (result == Z_OK) {
        decompressedData.resize(decompressedSize);
        qDebug() << "Decompression successful. Decompressed size:" << decompressedSize;
        return decompressedData;
    } else {
        qWarning() << "Zlib decompression failed with error code:" << result;
        if (result == Z_MEM_ERROR) {
            qWarning() << "Insufficient memory for decompression!";
        } else if (result == Z_BUF_ERROR) {
            qWarning() << "Output buffer was not large enough!";
        } else if (result == Z_DATA_ERROR) {
            qWarning() << "Corrupted input data!";
        }
        return {};
    }
}

struct MTXHeader {
    uint32_t magic;       // Magic number, always 0
    uint32_t lengthFirst; // Length of the first image
    uint32_t lengthSecond; // Length of the second image
};

QByteArray readSomeBytes(QFile &file, int number) {
    QByteArray b(number, '\0');
    qint64 bytesRead = file.read(b.data(), number);
    if (bytesRead != number) {
        qWarning() << "Failed to read enough bytes!";
    }
    return b;
}

void extractMTXv0(QFile &file) {
    // Read header (12 bytes)
    QByteArray headerData = readSomeBytes(file, 12);
    if (headerData.size() != 12) {
        qWarning() << "Error: Incorrect file size!";
        return;
    }

    MTXHeader header;
    header.magic = *reinterpret_cast<uint32_t *>(headerData.data());
    header.lengthFirst = *reinterpret_cast<uint32_t *>(headerData.data() + 4);
    header.lengthSecond = *reinterpret_cast<uint32_t *>(headerData.data() + 8);

    qDebug() << "Magic:" << header.magic
             << "LengthFirst:" << header.lengthFirst
             << "LengthSecond:" << header.lengthSecond;

    // Ensure the magic is 0 as per the MTXv0 specification
    if (header.magic != 0) {
        qWarning() << "Invalid magic number!";
        return;
    }

    // Extract the first image if it exists
    QByteArray firstImageData;
    if (header.lengthFirst > 0) {
        firstImageData = readSomeBytes(file, header.lengthFirst);
        qDebug() << "Extracting first image with size:" << header.lengthFirst;

        QString fileDir = QCoreApplication::applicationDirPath();
        QFileInfo fileInf(file.fileName());
        QString fileBase = fileInf.baseName();

        QString outFileName = fileDir + "/" + fileBase;

        qDebug() << "Filename:" << QString("%1.jpg").arg(outFileName);

        QFile firstImageFile(QString("%1.jpg").arg(outFileName));
        if (firstImageFile.open(QIODevice::WriteOnly)) {
            firstImageFile.write(firstImageData);
            firstImageFile.close();
        } else {
            qWarning() << "Failed to write first image!";
        }
    }

    // Extract the second image if it exists
    QByteArray secondImageData;
    if (header.lengthSecond > 0) {
        secondImageData = readSomeBytes(file, header.lengthSecond);
        qDebug() << "Extracting second image with size:" << header.lengthSecond;

        QFile secondImageFile(QString("%1_2.jpg").arg(file.fileName()));
        if (secondImageFile.open(QIODevice::WriteOnly)) {
            secondImageFile.write(secondImageData);
            secondImageFile.close();
        } else {
            qWarning() << "Failed to write second image!";
        }
    }

    qDebug() << "Extraction completed.";
}

// Function to extract images from MTXv1 files
void extractMTXv1(QFile &file, const QFileInfo &fileInfo) {
    QString fileDir = QCoreApplication::applicationDirPath();
    QString fileBaseNoExt = fileInfo.baseName();
    qint64 fileSize = fileInfo.size();

    // Example header reading (replace with actual implementation)
    QByteArray fileHeader = readSomeBytes(file, 64); // Assuming 64-byte header for simplicity

    int imageIndex = 1;
    qint64 filePos = 0;

    while (filePos < fileSize) {
        QString newOutFilePath = QString("%1/%2.png").arg(fileDir, fileBaseNoExt);
        qDebug() << "Extracting image" << imageIndex << "...";

        // Example block header reading (replace with actual implementation)
        QByteArray blockHeader = readSomeBytes(file, 16); // Assuming 16-byte block header for simplicity

        QByteArray chunkData = readSomeBytes(file, 1024); // Replace 1024 with actual chunk length

        // Decode the image
        QImage colorImage;
        if (!colorImage.loadFromData(chunkData)) {
            qWarning() << "Failed to decode image data.";
        }

        // Verify image dimensions (replace with actual header data)
        int expectedWidth = 256;
        int expectedHeight = 256;
        if (colorImage.width() != expectedWidth || colorImage.height() != expectedHeight) {
            qWarning() << "Image/header dimension mismatch detected.";
        }


        if (!colorImage.save(newOutFilePath, "PNG")) {
            qWarning() << "Failed to save image.";
        }


        filePos = file.pos();
        ++imageIndex;
    }

    qDebug() << "Done extracting mtx";
}

// Main extraction function
void extractMTXFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file.";
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.isFile()) {
        qWarning() << "Specified path is not a valid file.";
    }
    if (fileInfo.size() < 64) {
        qWarning() << "File is too small to be an MTX file.";
    }
    if (fileInfo.size() > MAX_INPUT_FILE_SIZE) {
        qWarning() << "File is larger than 1 GiB.";
    }

    // Read file version
    QByteArray fileVersionBytes = readSomeBytes(file, 4);
    quint32 fileVersion = *reinterpret_cast<const quint32 *>(fileVersionBytes.data());
    file.seek(0);

    switch (fileVersion) {
    case 0:
        qDebug() << "Format: MTXv0";
        extractMTXv0(file);
        break;
    case 1:
        qDebug() << "Format: MTXv1";
        extractMTXv1(file, fileInfo);
        break;
    default:
        qWarning() << "Unsupported MTX version.";
    }
}

#endif // TEXTUREEXTRACTOR_H
