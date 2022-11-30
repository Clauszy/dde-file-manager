/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<lanxuesong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dthumbnailprovider.h"
#include "dvideothumbnailprovider.h"
#include "dfm-base/mimetype/dmimedatabase.h"
#include "dfm-base/mimetype/mimetypedisplaymanager.h"
#include "dfm-base/base/standardpaths.h"
#include "dfm-base/utils/fileutils.h"
#include "dfm-base/base/application/application.h"
#include "dfm-base/base/schemefactory.h"
#include "dfm-base/utils/decorator/decoratorfile.h"
#include "dfm-base/utils/decorator/decoratorfileoperator.h"

#include <dfm-io/dfmio_utils.h>

#include <DThumbnailProvider>

#include <QCryptographicHash>
#include <QDir>
#include <QDateTime>
#include <QImageReader>
#include <QQueue>
#include <QMimeType>
#include <QReadWriteLock>
#include <QWaitCondition>
#include <QPainter>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QDebug>
#include <QtConcurrent>

// use original poppler api
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-image.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>

#include <sys/stat.h>

constexpr char kFormat[] { ".png" };

inline QByteArray dataToMd5Hex(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

namespace dfmbase {

class DThumbnailProviderPrivate
{
public:
    explicit DThumbnailProviderPrivate(DThumbnailProvider *qq);

    void init();

    QString sizeToFilePath(DThumbnailProvider::Size size) const;

    DThumbnailProvider *q = nullptr;
    QString errorString;
    qint64 defaultSizeLimit = 1024 * 1024 * 20;
    QHash<QMimeType, qint64> sizeLimitHash;
    DMimeDatabase mimeDatabase;
    QLibrary *libMovieViewer = nullptr;
    QHash<QString, QString> keyToThumbnailTool;
    QWaitCondition waitCondition;
    QMutex mutex;
    static QSet<QString> hasThumbnailMimeHash;

    struct ProduceInfo
    {
        QUrl url;
        DThumbnailProvider::CallBack callback = nullptr;
        DThumbnailProvider::Size size;
    };

    QQueue<ProduceInfo> produceQueue;
    QQueue<QString> produceAbsoluteFilePathQueue;

    bool running = true;
};

class DFileThumbnailProviderPrivate : public DThumbnailProvider
{
public:
    ~DFileThumbnailProviderPrivate();
};

DFileThumbnailProviderPrivate::~DFileThumbnailProviderPrivate()
{
}

}

using namespace dfmbase;

QSet<QString> DThumbnailProviderPrivate::hasThumbnailMimeHash;
Q_GLOBAL_STATIC(DFileThumbnailProviderPrivate, ftpGlobal)

DThumbnailProviderPrivate::DThumbnailProviderPrivate(DThumbnailProvider *qq)
    : q(qq)
{
}

void DThumbnailProviderPrivate::init()
{
    sizeLimitHash.reserve(28);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeTextPlain), 1024 * 1024);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeAppPdf), INT64_MAX);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeAppVRRMedia), INT64_MAX);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeAppVMAsf), INT64_MAX);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeAppMxf), INT64_MAX);

    //images
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeImageIef), 1024 * 1024 * 80);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeImageTiff), 1024 * 1024 * 80);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeImageXTMultipage), 1024 * 1024 * 80);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeImageVDMultipage), 1024 * 1024 * 80);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeImageXADng), 1024 * 1024 * 80);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeImageJpeg), 1024 * 1024 * 30);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeImagePng), 1024 * 1024 * 30);
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeImagePipeg), 1024 * 1024 * 30);
    // High file limit size only for FLAC files.
    sizeLimitHash.insert(mimeDatabase.mimeTypeForName(DFMGLOBAL_NAMESPACE::Mime::kTypeAudioFlac), INT64_MAX);
}

QString DThumbnailProviderPrivate::sizeToFilePath(DThumbnailProvider::Size size) const
{
    switch (size) {
    case DThumbnailProvider::Size::kSmall:
        return StandardPaths::location(StandardPaths::StandardLocation::kThumbnailSmallPath);
    case DThumbnailProvider::Size::kNormal:
        return StandardPaths::location(StandardPaths::StandardLocation::kThumbnailNormalPath);
    case DThumbnailProvider::Size::kLarge:
        return StandardPaths::location(StandardPaths::StandardLocation::kThumbnailLargePath);
    }
    return "";
}

DThumbnailProvider *DThumbnailProvider::instance()
{
    return ftpGlobal;
}

bool DThumbnailProvider::hasThumbnail(const QUrl &url) const
{
    const AbstractFileInfoPointer &fileInfo = InfoFactory::create<AbstractFileInfo>(url);

    if (!fileInfo->isReadable() || !fileInfo->isFile())
        return false;

    qint64 fileSize = fileInfo->size();

    if (fileSize <= 0)
        return false;

    const QMimeType &mime = d->mimeDatabase.mimeTypeForFile(url);

    // todo lanxs
    //if (mime.name().startsWith("video/") && FileJob::CopyingFiles.contains(QUrl::fromLocalFile(info.filePath())))
    //if (mime.name().startsWith("video/"))
    //  return false;

    if (mime.name().startsWith("video/") && DFMBASE_NAMESPACE::FileUtils::isGvfsFile(url))
        return false;

    if (fileSize > sizeLimit(mime) && !mime.name().startsWith("video/"))
        return false;

    return hasThumbnail(mime);
}

bool DThumbnailProvider::hasThumbnail(const QMimeType &mimeType) const
{
    const QString &mime = mimeType.name();
    QStringList mimeTypeList = { mime };
    mimeTypeList.append(mimeType.parentMimeTypes());

    if (mime.startsWith("image") && !Application::instance()->genericAttribute(Application::kPreviewImage).toBool())
        return false;

    if ((mime.startsWith("audio") || DFMBASE_NAMESPACE::MimeTypeDisplayManager::supportAudioMimeTypes().contains(mime))
        && !Application::instance()->genericAttribute(Application::kPreviewVideo).toBool())
        return false;

    if ((mime.startsWith("video")
         || DFMBASE_NAMESPACE::MimeTypeDisplayManager::supportVideoMimeTypes().contains(mime))
        && !Application::instance()->genericAttribute(Application::kPreviewVideo).toBool())
        return false;

    if (mime == DFMGLOBAL_NAMESPACE::Mime::kTypeTextPlain && !Application::instance()->genericAttribute(Application::kPreviewTextFile).toBool())
        return false;

    if (Q_LIKELY(mimeTypeList.contains(DFMGLOBAL_NAMESPACE::Mime::kTypeAppPdf)
                 || mime == DFMGLOBAL_NAMESPACE::Mime::kTypeAppCRRMedia
                 || mime == DFMGLOBAL_NAMESPACE::Mime::kTypeAppMxf)
        && !Application::instance()->genericAttribute(Application::kPreviewDocumentFile).toBool()) {
        return false;
    }

    if (DThumbnailProviderPrivate::hasThumbnailMimeHash.contains(mime))
        return true;

    if (Q_LIKELY(mime.startsWith("image") || mime.startsWith("audio/") || mime.startsWith("video/"))) {
        DThumbnailProviderPrivate::hasThumbnailMimeHash.insert(mime);

        return true;
    }

    if (Q_LIKELY(mime == DFMGLOBAL_NAMESPACE::Mime::kTypeTextPlain
                 || mimeTypeList.contains(DFMGLOBAL_NAMESPACE::Mime::kTypeAppPdf)
                 || mime == DFMGLOBAL_NAMESPACE::Mime::kTypeAppVRRMedia
                 || mime == DFMGLOBAL_NAMESPACE::Mime::kTypeAppVMAsf
                 || mime == DFMGLOBAL_NAMESPACE::Mime::kTypeAppMxf)) {
        DThumbnailProviderPrivate::hasThumbnailMimeHash.insert(mime);

        return true;
    }

    if (DTK_GUI_NAMESPACE::DThumbnailProvider::instance()->hasThumbnail(mimeType))
        return true;

    return false;
}

// true 1, false 0, invalid -1
int DThumbnailProvider::hasThumbnailFast(const QString &mime) const
{
    if (mime.startsWith("image") && !Application::instance()->genericAttribute(Application::kPreviewImage).toBool())
        return 0;

    if ((mime.startsWith("video")
         || DFMBASE_NAMESPACE::MimeTypeDisplayManager::supportVideoMimeTypes().contains(mime))
        && !Application::instance()->genericAttribute(Application::kPreviewVideo).toBool())
        return 0;

    if ((mime.startsWith("audio")
         || DFMBASE_NAMESPACE::MimeTypeDisplayManager::supportAudioMimeTypes().contains(mime))
        && !Application::instance()->genericAttribute(Application::kPreviewVideo).toBool())
        return 0;

    if (mime == DFMGLOBAL_NAMESPACE::Mime::kTypeTextPlain && !Application::instance()->genericAttribute(Application::kPreviewTextFile).toBool())
        return 0;

    if (Q_LIKELY(mime == DFMGLOBAL_NAMESPACE::Mime::kTypeAppCRRMedia
                 || mime == DFMGLOBAL_NAMESPACE::Mime::kTypeAppMxf)
        && !Application::instance()->genericAttribute(Application::kPreviewDocumentFile).toBool()) {
        return 0;
    }

    if (DThumbnailProviderPrivate::hasThumbnailMimeHash.contains(mime))
        return 1;

    if (Q_LIKELY(mime.startsWith("image") || mime.startsWith("audio/") || mime.startsWith("video/"))) {
        DThumbnailProviderPrivate::hasThumbnailMimeHash.insert(mime);

        return 1;
    }

    if (Q_LIKELY(mime == DFMGLOBAL_NAMESPACE::Mime::kTypeTextPlain
                 || mime == DFMGLOBAL_NAMESPACE::Mime::kTypeAppVRRMedia
                 || mime == DFMGLOBAL_NAMESPACE::Mime::kTypeAppVMAsf
                 || mime == DFMGLOBAL_NAMESPACE::Mime::kTypeAppMxf)) {
        DThumbnailProviderPrivate::hasThumbnailMimeHash.insert(mime);

        return 1;
    }

    return -1;
}

QString DThumbnailProvider::thumbnailFilePath(const QUrl &fileUrl, Size size) const
{
    AbstractFileInfoPointer fileInfo = InfoFactory::create<AbstractFileInfo>(fileUrl);
    if (!fileInfo)
        return QString();

    const QString &absolutePath = fileInfo->path();
    const QString &absoluteFilePath = fileInfo->filePath();

    if (absolutePath == d->sizeToFilePath(DThumbnailProvider::Size::kSmall)
        || absolutePath == d->sizeToFilePath(DThumbnailProvider::Size::kNormal)
        || absolutePath == d->sizeToFilePath(DThumbnailProvider::Size::kLarge)
        || absolutePath == StandardPaths::location(StandardPaths::kThumbnailFailPath)) {
        return absoluteFilePath;
    }

    const QString thumbnailName = dataToMd5Hex((QUrl::fromLocalFile(absoluteFilePath).toString(QUrl::FullyEncoded)).toLocal8Bit()) + kFormat;
    QString thumbnail = DFMIO::DFMUtils::buildFilePath(d->sizeToFilePath(size).toStdString().c_str(), thumbnailName.toStdString().c_str(), nullptr);
    if (!DecoratorFile(thumbnail).exists()) {
        return QString();
    }

    QImageReader ir(thumbnail, QByteArray(kFormat).mid(1));
    if (!ir.canRead()) {
        DecoratorFileOperator(thumbnail).deleteFile();
        return QString();
    }
    ir.setAutoDetectImageFormat(false);

    const QImage image = ir.read();

    const qint64 fileModify = fileInfo->lastModified().toSecsSinceEpoch();
    if (!image.isNull() && image.text(QT_STRINGIFY(Thumb::MTime)).toInt() != static_cast<int>(fileModify)) {
        DecoratorFileOperator(thumbnail).deleteFile();

        return QString();
    }

    return thumbnail;
}

static QString generalKey(const QString &key)
{
    const QStringList &tmp = key.split('/');

    if (tmp.size() > 1)
        return tmp.first() + "/*";

    return key;
}

QString DThumbnailProvider::createThumbnail(const QUrl &url, DThumbnailProvider::Size size)
{
    d->errorString.clear();

    const AbstractFileInfoPointer &fileInfo = InfoFactory::create<AbstractFileInfo>(url);

    const QString &DirPath = fileInfo->absolutePath();
    const QString &filePath = fileInfo->absoluteFilePath();

    if (DirPath == d->sizeToFilePath(DThumbnailProvider::Size::kSmall)
        || DirPath == d->sizeToFilePath(DThumbnailProvider::Size::kNormal)
        || DirPath == d->sizeToFilePath(DThumbnailProvider::Size::kLarge)
        || DirPath == StandardPaths::location(StandardPaths::kThumbnailFailPath)) {
        return filePath;
    }

    if (!hasThumbnail(url)) {
        d->errorString = QStringLiteral("This file has not support thumbnail: ") + filePath;

        //!Warnning: Do not store thumbnails to the fail path
        return QString();
    }

    const QString fileUrl = QUrl::fromLocalFile(filePath).toString(QUrl::FullyEncoded);
    const QString thumbnailName = dataToMd5Hex(fileUrl.toLocal8Bit()) + kFormat;

    // the file is in fail path
    QString thumbnail = DFMIO::DFMUtils::buildFilePath(StandardPaths::location(StandardPaths::kThumbnailFailPath).toStdString().c_str(),
                                                       thumbnailName.toStdString().c_str(), nullptr);

    QMimeType mime = d->mimeDatabase.mimeTypeForFile(url);
    QScopedPointer<QImage> image(new QImage());

    QStringList mimeTypeList = { mime.name() };
    mimeTypeList.append(mime.parentMimeTypes());

    //! 新增djvu格式文件缩略图预览
    if (mime.name().contains(DFMGLOBAL_NAMESPACE::Mime::kTypeImageVDjvu)) {
        if (createImageVDjvuThumbnail(filePath, size, image, thumbnailName, thumbnail))
            return thumbnail;
    } else if (mime.name().startsWith("image/")) {
        createImageThumbnail(url, mime, filePath, size, image);
    } else if (mime.name() == DFMGLOBAL_NAMESPACE::Mime::kTypeTextPlain) {
        createTextThumbnail(filePath, size, image);
    } else if (mimeTypeList.contains(DFMGLOBAL_NAMESPACE::Mime::kTypeAppPdf)) {
        createPdfThumbnail(filePath, size, image);
    } else if (mime.name().startsWith("audio/")) {
        createAudioThumbnail(filePath, size, image);
    } else {
        if (createDefaultThumbnail(mime, filePath, size, image, thumbnail))
            return thumbnail;
    }

    // successful
    if (d->errorString.isEmpty()) {
        thumbnail = DFMIO::DFMUtils::buildFilePath(d->sizeToFilePath(size).toStdString().c_str(), thumbnailName.toStdString().c_str(), nullptr);
    } else {
        //fail
        image.reset(new QImage(1, 1, QImage::Format_Mono));
    }

    image->setText(QT_STRINGIFY(Thumb::URL), fileUrl);
    const qint64 fileModify = fileInfo->lastModified().toSecsSinceEpoch();
    image->setText(QT_STRINGIFY(Thumb::MTime), QString::number(fileModify));

    // create path
    QFileInfo(thumbnail).absoluteDir().mkpath(".");

    if (!image->save(thumbnail, Q_NULLPTR, 80)) {
        d->errorString = QStringLiteral("Can not save image to ") + thumbnail;
    }

    if (d->errorString.isEmpty()) {
        emit createThumbnailFinished(filePath, thumbnail);

        return thumbnail;
    }

    // fail
    emit createThumbnailFailed(filePath);

    return QString();
}

void DThumbnailProvider::createAudioThumbnail(const QString &filePath, DThumbnailProvider::Size size, QScopedPointer<QImage> &image)
{
    QProcess ffmpeg;
    ffmpeg.start("ffmpeg", QStringList() << "-nostats"
                                         << "-loglevel"
                                         << "0"
                                         << "-i" << QDir::toNativeSeparators(filePath) << "-an"
                                         << "-vf"
                                         << "scale='min(" + QString::number(size) + ",iw)':-1"
                                         << "-f"
                                         << "image2pipe"
                                         << "-fs"
                                         << "9000"
                                         << "-",
                 QIODevice::ReadOnly);

    if (!ffmpeg.waitForFinished()) {
        d->errorString = ffmpeg.errorString();
        return;
    }

    const QByteArray &output = ffmpeg.readAllStandardOutput();

    if (image->loadFromData(output)) {
        d->errorString.clear();
    } else {
        d->errorString = QString("load image failed from the ffmpeg application");
    }
}

bool DThumbnailProvider::createImageVDjvuThumbnail(const QString &filePath, DThumbnailProvider::Size size, QScopedPointer<QImage> &image,
                                                   const QString &thumbnailName, QString &thumbnail)
{
    thumbnail = DTK_GUI_NAMESPACE::DThumbnailProvider::instance()->createThumbnail(QFileInfo(filePath),
                                                                                   static_cast<DTK_GUI_NAMESPACE::DThumbnailProvider::Size>(size));
    d->errorString = DTK_GUI_NAMESPACE::DThumbnailProvider::instance()->errorString();

    if (d->errorString.isEmpty()) {
        emit createThumbnailFinished(filePath, thumbnail);

        return true;
    } else {
        const QString &readerBinary = QStandardPaths::findExecutable("deepin-reader");
        if (readerBinary.isEmpty())
            return true;
        //! 使用子进程来调用deepin-reader程序生成djvu格式文件缩略图
        QProcess process;
        QStringList arguments;
        //! 生成缩略图缓存地址
        const QString &saveImage = DFMIO::DFMUtils::buildFilePath(d->sizeToFilePath(size).toStdString().c_str(),
                                                                  thumbnailName.toStdString().c_str(), nullptr);
        arguments << "--thumbnail"
                  << "-f" << filePath << "-t" << saveImage;
        process.start(readerBinary, arguments);

        if (!process.waitForFinished()) {
            d->errorString = process.errorString();

            return false;
        }

        if (process.exitCode() != 0) {
            const QString &error = process.readAllStandardError();

            if (error.isEmpty()) {
                d->errorString = QString("get thumbnail failed from the \"%1\" application").arg(readerBinary);
            } else {
                d->errorString = error;
            }

            return false;
        }

        auto dfile = DFMBASE_NAMESPACE::DecoratorFile(saveImage).filePtr();
        if (dfile && dfile->open(DFMIO::DFile::OpenFlag::kReadOnly)) {
            const QByteArray &output = dfile->readAll();
            Q_ASSERT(!output.isEmpty());

            if (image->loadFromData(output, "png")) {
                d->errorString.clear();
            }
            dfile->close();
        }
    }
    return false;
}

void DThumbnailProvider::createImageThumbnail(const QUrl &url, const QMimeType &mime, const QString &filePath, DThumbnailProvider::Size size, QScopedPointer<QImage> &image)
{
    //! fix bug#49451 因为使用mime.preferredSuffix(),会导致后续image.save崩溃，具体原因还需进一步跟进
    //! QImageReader构造时不传format参数，让其自行判断
    //! fix bug #53200 QImageReader构造时不传format参数，会造成没有读取不了真实的文件 类型比如将png图标后缀修改为jpg，读取的类型不对

    QString mimeType = d->mimeDatabase.mimeTypeForFile(url, QMimeDatabase::MatchContent).name();
    QString suffix = mimeType.replace("image/", "");

    QImageReader reader(filePath, suffix.toLatin1());
    if (!reader.canRead()) {
        d->errorString = reader.errorString();
        return;
    }

    const QSize &imageSize = reader.size();

    //fix 读取损坏icns文件（可能任意损坏的image类文件也有此情况）在arm平台上会导致递归循环的问题
    //这里先对损坏文件（imagesize无效）做处理，不再尝试读取其image数据
    if (!imageSize.isValid()) {
        d->errorString = "Fail to read image file attribute data:" + filePath;
        return;
    }

    if (imageSize.width() > size || imageSize.height() > size || mime.name() == DFMGLOBAL_NAMESPACE::Mime::kTypeImageSvgXml) {
        reader.setScaledSize(reader.size().scaled(size, size, Qt::KeepAspectRatio));
    }

    reader.setAutoTransform(true);

    if (!reader.read(image.data())) {
        d->errorString = reader.errorString();
        return;
    }

    if (image->width() > size || image->height() > size) {
        image->operator=(image->scaled(size, size, Qt::KeepAspectRatio));
    }
}

void DThumbnailProvider::createTextThumbnail(const QString &filePath, DThumbnailProvider::Size size, QScopedPointer<QImage> &image)
{
    //FIXME(zccrs): This should be done using the image plugin?
    auto dfile = DFMBASE_NAMESPACE::DecoratorFile(filePath).filePtr();
    if (!dfile || !dfile->open(DFMIO::DFile::OpenFlag::kReadOnly)) {
        d->errorString = dfile->lastError().errorMsg();
        return;
    }
    AbstractFileInfoPointer fileinfo = InfoFactory::create<AbstractFileInfo>(QUrl::fromLocalFile(filePath));
    if (!fileinfo)
        return;

    QString text { FileUtils::toUnicode(dfile->read(2000), fileinfo->fileName()) };

    QFont font;
    font.setPixelSize(12);

    QPen pen;
    pen.setColor(Qt::black);

    *image = QImage(static_cast<int>(0.70707070 * size), size, QImage::Format_ARGB32_Premultiplied);
    image->fill(Qt::white);

    QPainter painter(image.data());
    painter.setFont(font);
    painter.setPen(pen);

    QTextOption option;

    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    painter.drawText(image->rect(), text, option);
}

void DThumbnailProvider::createPdfThumbnail(const QString &filePath, DThumbnailProvider::Size size, QScopedPointer<QImage> &image)
{
    //FIXME(zccrs): This should be done using the image plugin?
    QScopedPointer<poppler::document> doc(poppler::document::load_from_file(filePath.toStdString()));

    if (!doc || doc->is_locked()) {
        d->errorString = QStringLiteral("Cannot read this pdf file: ") + filePath;
        return;
    }

    if (doc->pages() < 1) {
        d->errorString = QStringLiteral("This stream is invalid");
        return;
    }

    QScopedPointer<const poppler::page> page(doc->create_page(0));

    if (!page) {
        d->errorString = QStringLiteral("Cannot get this page at index 0");
        return;
    }

    poppler::page_renderer pr;
    pr.set_render_hint(poppler::page_renderer::antialiasing, true);
    pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);

    poppler::image imageData = pr.render_page(page.data(), 72, 72, -1, -1, -1, size);

    if (!imageData.is_valid()) {
        d->errorString = QStringLiteral("Render error");
        return;
    }

    poppler::image::format_enum format = imageData.format();
    QImage img;

    switch (format) {
    case poppler::image::format_invalid:
        d->errorString = QStringLiteral("Image format is invalid");
        return;
    case poppler::image::format_mono:
        img = QImage(reinterpret_cast<uchar *>(imageData.data()), imageData.width(), imageData.height(), QImage::Format_Mono);
        break;
    case poppler::image::format_rgb24:
        img = QImage(reinterpret_cast<uchar *>(imageData.data()), imageData.width(), imageData.height(), QImage::Format_ARGB6666_Premultiplied);
        break;
    case poppler::image::format_argb32:
        img = QImage(reinterpret_cast<uchar *>(imageData.data()), imageData.width(), imageData.height(), QImage::Format_ARGB32);
        break;
    default:
        break;
    }

    if (img.isNull()) {
        d->errorString = QStringLiteral("Render error");
        return;
    }

    *image = img.scaled(QSize(size, size), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

bool DThumbnailProvider::createDefaultThumbnail(const QMimeType &mime, const QString &filePath, DThumbnailProvider::Size size, QScopedPointer<QImage> &image, QString &thumbnail)
{
    if (createThumnailByMovieLib(filePath, image))
        return false;

    thumbnail = DTK_GUI_NAMESPACE::DThumbnailProvider::instance()->createThumbnail(QFileInfo(filePath), static_cast<DTK_GUI_NAMESPACE::DThumbnailProvider::Size>(size));
    d->errorString = DTK_GUI_NAMESPACE::DThumbnailProvider::instance()->errorString();

    if (d->errorString.isEmpty()) {
        emit createThumbnailFinished(filePath, thumbnail);

        return true;
    }

    if (!createThumnailByDtkTools(mime, size, filePath, image))
        return createThumnailByTools(mime, size, filePath, image);

    return false;
}

bool DThumbnailProvider::createThumnailByMovieLib(const QString &filePath, QScopedPointer<QImage> &image)
{
    //获取缩略图生成库函数getMovieCover的指针
    if (!d->libMovieViewer || !d->libMovieViewer->isLoaded()) {
        d->libMovieViewer = new QLibrary("libimageviewer.so");
        d->libMovieViewer->load();
    }
    if (d->libMovieViewer && d->libMovieViewer->isLoaded()) {
        typedef void (*getMovieCover)(const QUrl &url, const QString &savePath, QImage *imageRet);
        getMovieCover func = reinterpret_cast<void (*)(const QUrl &, const QString &, QImage *)>(d->libMovieViewer->resolve("getMovieCover"));
        if (func) {   //存在导出函数getMovieCover
            auto url = QUrl::fromLocalFile(filePath);
            QImage img;
            func(url, filePath, &img);   //调用getMovieCover生成缩略图
            if (!img.isNull()) {
                *image = img;
                d->errorString.clear();
                return true;
            }
        }
    }
    return false;
}

void DThumbnailProvider::initThumnailTool()
{
#ifdef THUMBNAIL_TOOL_DIR
    if (d->keyToThumbnailTool.isEmpty()) {
        d->keyToThumbnailTool["Initialized"] = QString();

        for (const QString &path : QString(THUMBNAIL_TOOL_DIR).split(":")) {
            const QString &thumbnailToolPath = DFMIO::DFMUtils::buildFilePath(path.toStdString().c_str(), "thumbnail", nullptr);
            QDirIterator dir(thumbnailToolPath, { "*.json" }, QDir::NoDotAndDotDot | QDir::Files);

            while (dir.hasNext()) {
                const QString &filePath = dir.next();
                const QFileInfo &fileInfo = dir.fileInfo();

                QFile file(filePath);

                if (!file.open(QFile::ReadOnly)) {
                    continue;
                }

                const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
                file.close();

                const QStringList keys = document.object().toVariantMap().value("Keys").toStringList();
                const QString &toolFilePath = fileInfo.absoluteDir().filePath(fileInfo.baseName());

                if (!QFile::exists(toolFilePath)) {
                    continue;
                }

                for (const QString &key : keys) {
                    if (d->keyToThumbnailTool.contains(key))
                        continue;

                    d->keyToThumbnailTool[key] = toolFilePath;
                }
            }
        }
    }
#endif
}

bool DThumbnailProvider::createThumnailByDtkTools(const QMimeType &mime, DThumbnailProvider::Size size, const QString &filePath, QScopedPointer<QImage> &image)
{
    DFMBASE_NAMESPACE::DVideoThumbnailProvider videoProvider;

    QString mimeName = mime.name();
    bool useVideo = videoProvider.hasKey(mimeName);
    if (!useVideo) {
        mimeName = generalKey(mimeName);
        useVideo = videoProvider.hasKey(mimeName);
    }
    if (useVideo) {
        *image = videoProvider.createThumbnail(QString::number(size), filePath);
        d->errorString.clear();
        return true;
    }

    return false;
}

bool DThumbnailProvider::createThumnailByTools(const QMimeType &mime, DThumbnailProvider::Size size, const QString &filePath, QScopedPointer<QImage> &image)
{
    initThumnailTool();
    QString mimeName = mime.name();
    QString tool = d->keyToThumbnailTool.value(mimeName);

    if (tool.isEmpty()) {
        mimeName = generalKey(mimeName);
        tool = d->keyToThumbnailTool.value(mimeName);
    }

    if (tool.isEmpty()) {
        return true;
    }

    QProcess process;
    process.start(tool, { QString::number(size), filePath }, QIODevice::ReadOnly);

    if (!process.waitForFinished()) {
        d->errorString = process.errorString();

        return false;
    }

    if (process.exitCode() != 0) {
        const QString &error = process.readAllStandardError();

        if (error.isEmpty()) {
            d->errorString = QString("get thumbnail failed from the \"%1\" application").arg(tool);
        } else {
            d->errorString = error;
        }

        return false;
    }

    const QByteArray output = process.readAllStandardOutput();
    const QByteArray pngData = QByteArray::fromBase64(output);
    Q_ASSERT(!pngData.isEmpty());

    if (image->loadFromData(pngData, "png")) {
        d->errorString.clear();
    } else {
        // 过滤video tool的其他输出信息
        QString processResult(output);
        processResult = processResult.split(QRegExp("[\n]"), QString::SkipEmptyParts).last();
        const QByteArray pngData = QByteArray::fromBase64(processResult.toUtf8());
        Q_ASSERT(!pngData.isEmpty());
        if (image->loadFromData(pngData, "png")) {
            d->errorString.clear();
        } else {
            d->errorString = QString("load png image failed from the \"%1\" application").arg(tool);
        }
    }
    return false;
}

void DThumbnailProvider::appendToProduceQueue(const QUrl &url, DThumbnailProvider::Size size, DThumbnailProvider::CallBack callback)
{
    DThumbnailProviderPrivate::ProduceInfo produceInfo;

    produceInfo.url = url;
    produceInfo.size = size;
    produceInfo.callback = callback;

    {
        QMutexLocker locker(&d->mutex);
        // fix bug 62540 这里在没生成缩略图的情况下，（触发刷新，文件大小改变）同一个文件会多次生成缩略图的情况,
        // 缓存当前队列中生成的缩略图文件的路劲没有就加入队里生成
        if (!d->produceAbsoluteFilePathQueue.contains(url.path())) {
            d->produceQueue.append(std::move(produceInfo));
            d->produceAbsoluteFilePathQueue.push_back(url.path());
        }
    }

    if (!isRunning())
        start();
    else
        d->waitCondition.wakeAll();
}

QString DThumbnailProvider::errorString() const
{
    return d->errorString;
}

qint64 DThumbnailProvider::sizeLimit(const QMimeType &mimeType) const
{
    return d->sizeLimitHash.value(mimeType, d->defaultSizeLimit);
}

DThumbnailProvider::DThumbnailProvider(QObject *parent)
    : QThread(parent), d(new DThumbnailProviderPrivate(this))
{
    d->init();
}

DThumbnailProvider::~DThumbnailProvider()
{
    d->running = false;
    d->waitCondition.wakeAll();
    wait();
    if (d->libMovieViewer && d->libMovieViewer->isLoaded()) {
        d->libMovieViewer->unload();
        delete d->libMovieViewer;
        d->libMovieViewer = nullptr;
    }
}

void DThumbnailProvider::run()
{
    forever {

        if (d->produceQueue.isEmpty()) {
            d->mutex.lock();
            d->waitCondition.wait(&d->mutex);
            d->mutex.unlock();
        }

        if (!d->running)
            return;

        QMutexLocker locker(&d->mutex);
        const DThumbnailProviderPrivate::ProduceInfo &task = d->produceQueue.dequeue();

        locker.unlock();
        const QString &thumbnail = createThumbnail(task.url, task.size);

        locker.relock();
        //fix 62540 生成结束了去除缓存
        d->produceAbsoluteFilePathQueue.removeOne(task.url.path());
        locker.unlock();

        if (task.callback)
            task.callback(thumbnail);
    }
}
