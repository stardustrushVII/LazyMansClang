// (c) 2025 Stardust Softworks
#include "aboutdialog.h"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QOperatingSystemVersion>
#include <QPixmap>
#include <QPushButton>
#include <QTextBrowser>
#include <QtGlobal>

#ifdef __clang__
#define LMC_COMPILER  QString("Clang %1").arg(__clang_major__)
#elif defined(__GNUC__)
#define LMC_COMPILER  QString("GCC %1.%2").arg(__GNUC__).arg(__GNUC_MINOR__)
#elif defined(_MSC_VER)
#define LMC_COMPILER  QString("MSVC %1").arg(_MSC_VER)
#else
#define LMC_COMPILER  QString("Unknown Compiler")
#endif

static QString archString() {
#if defined(Q_PROCESSOR_ARM_64)
    return "arm64";
#elif defined(Q_PROCESSOR_X86_64)
    return "x86_64";
#else
    return QSysInfo::currentCpuArchitecture();
#endif
}

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("About %1").arg(QCoreApplication::applicationName()));
    setModal(true);
    resize(680, 440);   // nice and roomy

    auto* grid = new QGridLayout(this);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(10);
    grid->setContentsMargins(16,16,16,16);

    // logo on the left
    logoLabel = new QLabel(this);
    QPixmap pm(":/icns/256x256.png");               // 256x256 res
    if (!pm.isNull()) pm = pm.scaled(180,180, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    logoLabel->setPixmap(pm);
    logoLabel->setMinimumSize(128,128);
    logoLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    // information on right
    titleLabel = new QLabel(this);
    titleLabel->setTextFormat(Qt::RichText);
    titleLabel->setText(QString("<div style='font-size:20px; font-weight:600'>%1</div>")
                            .arg(QCoreApplication::applicationName()));

    subtitleLabel = new QLabel(this);
    subtitleLabel->setTextFormat(Qt::RichText);
    const QString ver = QCoreApplication::applicationVersion().isEmpty()
                            ? QStringLiteral("—")
                            : QCoreApplication::applicationVersion();
    subtitleLabel->setText(
        QString("<div style='color:#777'>Version %1</div>"
                "<div style='color:#777'>%2</div>")
            .arg(ver, buildInfo())
        );

    // credits
    credits = new QTextBrowser(this);
    credits->setOpenExternalLinks(true);
    credits->setHtml(
        "<p><b>LazyMansClang</b> — a simple GUI front-end for clang++.</p>"
        "<p>© 2025 <b>Stardust Softworks</b></p>"
        "<p><a href='https://stardustsoftworks.com'>stardustsoftworks.com</a></p>"
        "<hr>"
        "<p>Built with Qt. © The Qt Company.</p>"
        );

    // buttons
    auto* buttons = new QDialogButtonBox(this);
    copyBtn  = buttons->addButton(tr("Copy Info"), QDialogButtonBox::ActionRole);
    closeBtn = buttons->addButton(QDialogButtonBox::Close);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(copyBtn,  &QPushButton::clicked, this, [this]{ copyAllToClipboard(); });

    // layout arr
    int r = 0;
    grid->addWidget(logoLabel,     r,   0, 3, 1);
    grid->addWidget(titleLabel,    r,   1, 1, 1);
    grid->addWidget(subtitleLabel, r+1, 1, 1, 1);
    grid->addWidget(credits,       r+2, 1, 1, 1);
    grid->addWidget(buttons,       r+3, 0, 1, 2);
}

QString AboutDialog::buildInfo() const {
    const QString qtVer = QString::fromLatin1(qVersion());
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    const QString qtMajor = "Qt 6";
#else
    const QString qtMajor = "Qt 5";
#endif
    const QString os = QOperatingSystemVersion::current().name();
    const QString when = QString("%1 %2").arg(__DATE__).arg(__TIME__);
    return QString("%1 (%2), %3, %4, built %5")
        .arg(qtMajor, qtVer, LMC_COMPILER, archString(), when);
}

void AboutDialog::setCreditsHtml(const QString& html) {
    credits->setHtml(html);
}

void AboutDialog::setExtraBuildLine(const QString& line) {
    if (line.isEmpty()) return;
    auto t = subtitleLabel->text();
    t += QString("<div style='color:#777'>%1</div>").arg(line.toHtmlEscaped());
    subtitleLabel->setText(t);
}

void AboutDialog::copyAllToClipboard() const {
    QString text;
    text += QString("%1 %2\n")
                .arg(QCoreApplication::applicationName(),
                     QCoreApplication::applicationVersion());
    text += buildInfo() + "\n\n";
    text += credits->toPlainText();
    QApplication::clipboard()->setText(text);
}
