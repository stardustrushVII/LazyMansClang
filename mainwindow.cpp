// (c) 2025 Stardust Softworks
#include "mainwindow.h"
#include "aboutdialog.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QTextStream>
#include <QMessageBox>
#include <QTextCursor>
#include <QListWidgetItem>
#include <QFile>
#include <QFileDevice>
#include <QRegularExpression>
#include <QAction>
#include <QMenuBar>

// software plumbing (wiring)
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("LazyMansClang");

    auto* aboutAct = new QAction(tr("About %1").arg(QCoreApplication::applicationName()), this);
    aboutAct->setMenuRole(QAction::AboutRole);         // macOS app menu
    connect(aboutAct, &QAction::triggered, this, [this]{
        AboutDialog dlg(this);
        dlg.exec();
        //dlg.setExtraBuildLine("rev xxxxxxxxx");
    });

    // add to any menu -- Qt(sucks) will relocate on macOS
    QMenu* appMenu = menuBar()->addMenu(tr("&App"));
    appMenu->addAction(aboutAct);

    // signals | slots
    connect(ui->addFilesButton, &QPushButton::clicked, this, &MainWindow::addFiles);
    connect(ui->removeFilesButton, &QPushButton::clicked, this, &MainWindow::removeSelectedFiles);
    connect(ui->clearFilesButton, &QPushButton::clicked, this, &MainWindow::clearFiles);
    connect(ui->browseCompilerButton, &QPushButton::clicked, this, &MainWindow::browseCompiler);
    connect(ui->browseOutputPath, &QPushButton::clicked, this, &MainWindow::browseOutputPath);
    connect(ui->buildButton, &QPushButton::clicked, this, &MainWindow::buildProject);
    connect(ui->cleanButton, &QPushButton::clicked, this, &MainWindow::cleanBuild);

#ifdef Q_OS_WIN
    ui->compilerPathInput->setText("clang++.exe");
#else
    ui->compilerPathInput->setText("/usr/bin/clang++");
#endif
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::appendLog(const QString &s) {
    ui->outputBox->moveCursor(QTextCursor::End);
    ui->outputBox->insertPlainText(s);
    ui->outputBox->moveCursor(QTextCursor::End);
}

QStringList MainWindow::parseLines(const QString &text) const {
    QStringList lines;
    for (const QString &raw : text.split('\n')) {
        const QString t = raw.trimmed();
        if (!t.isEmpty()) lines << t;
    }
    return lines;
}

QString MainWindow::compilerCmd() const {
    QString cmd = ui->compilerPathInput->text().trimmed();
#ifdef Q_OS_MAC
    if (cmd.isEmpty()) cmd = "/usr/bin/clang++";
#elif defined(Q_OS_WIN)
    if (cmd.isEmpty()) cmd = "clang++.exe";
#else
    if (cmd.isEmpty()) cmd = "clang++";
#endif
    return cmd;
}

// MS Windows: .exe; macOS/Linux(Unix): **no extension** (strip .out if typed)
QString MainWindow::targetPathWithExt(QString out) const {
#ifdef Q_OS_WIN
    if (!out.endsWith(".exe", Qt::CaseInsensitive)) out += ".exe";
#else
    if (out.endsWith(".out", Qt::CaseInsensitive)) out.chop(4);
#endif
    return out;
}

QString MainWindow::buildDirForTarget(const QString &target) const {
    QFileInfo t(target);
    QDir d = t.dir();
    QString b = d.absoluteFilePath("build");
    QDir().mkpath(b);
    return b;
}

// file list actions
void MainWindow::addFiles() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this, "Add Source Files", QString(),
        "C/C++ Sources (*.cpp *.cc *.c);;Headers (*.h *.hpp);;All Files (*)");
    for (const QString &f : files) {
        if (f.isEmpty()) continue;
        if (ui->fileList->findItems(f, Qt::MatchExactly).isEmpty())
            ui->fileList->addItem(f);
    }
}

void MainWindow::removeSelectedFiles() { qDeleteAll(ui->fileList->selectedItems()); }
void MainWindow::clearFiles() { ui->fileList->clear(); }

void MainWindow::browseCompiler() {
    QString f = QFileDialog::getOpenFileName(
        this, "Select C++ Compiler", QString(),
#ifdef Q_OS_WIN
        "Executables (*.exe);;All Files (*)"
#else
        "All Files (*)"
#endif
        );
    if (!f.isEmpty()) ui->compilerPathInput->setText(f);
}

void MainWindow::browseOutputPath() {
    QString f = QFileDialog::getSaveFileName(
        this, "Choose Output File", QString(),
#ifdef Q_OS_WIN
        "Executable (*.exe);;All Files (*)" // Windows exe
#else
        "Executable (*);;All Files (*)"   // allows native unix exec's
#endif
        );
    if (!f.isEmpty()) ui->outputPathInput->setText(f);
}

// process run (handles stdout + stderr)
bool MainWindow::runProcess(const QString &program, const QStringList &args,
                            QString *stdoutOut, QString *stderrOut)
{
    QProcess p;
    p.setProgram(program);
    p.setArguments(args);
    p.setProcessChannelMode(QProcess::SeparateChannels);

    p.start();
    if (!p.waitForStarted()) {
        appendLog("\n❌ Failed to start: " + program + "\n");
        return false;
    }

    while (p.state() != QProcess::NotRunning) {
        p.waitForReadyRead(50);
        const QByteArray outData = p.readAllStandardOutput();
        const QByteArray errData = p.readAllStandardError();
        if (!outData.isEmpty()) appendLog(QString::fromLocal8Bit(outData));
        if (!errData.isEmpty()) appendLog(QString::fromLocal8Bit(errData));
    }

    // drains
    const QByteArray outData = p.readAllStandardOutput();
    const QByteArray errData = p.readAllStandardError();
    if (!outData.isEmpty()) appendLog(QString::fromLocal8Bit(outData));
    if (!errData.isEmpty()) appendLog(QString::fromLocal8Bit(errData));

    if (stdoutOut) *stdoutOut = QString::fromLocal8Bit(outData);
    if (stderrOut) *stderrOut = QString::fromLocal8Bit(errData);

    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        appendLog("\nExited with code " + QString::number(p.exitCode()) + "\n");
        return false;
    }
    return true;
}

// auto-detect helpers (SDL2, GLFW, SFML)
static bool lmc_containsSwitch(const QStringList& xs, const QString& needlePrefix) {
    for (const QString& x : xs) if (x.startsWith(needlePrefix)) return true;
    return false;
}

static QStringList lmc_splitArgs(const QString& s) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    return QProcess::splitCommand(s);
#else
    return s.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
#endif
}

static QString lmc_runShell(const QString& cmd) {
#ifdef Q_OS_WIN
    Q_UNUSED(cmd)
    return QString();
#else
    QProcess sh;
    sh.start("/bin/sh", {"-c", cmd});
    sh.waitForFinished(-1);
    return QString::fromLocal8Bit(sh.readAllStandardOutput()).trimmed();
#endif
}

// pkg-config: returns {cflags, libs}
static QPair<QStringList, QStringList> lmc_pkgConfigFlags(const QString& pkg) {
    QStringList cflags, libs;
#ifndef Q_OS_WIN
    const QString c = lmc_runShell("pkg-config --cflags " + pkg);
    const QString l = lmc_runShell("pkg-config --libs "   + pkg);
    if (!c.isEmpty()) cflags = lmc_splitArgs(c);
    if (!l.isEmpty()) libs   = lmc_splitArgs(l);
#endif
    return {cflags, libs};
}

// sdl2-config
static QPair<QStringList, QStringList> lmc_sdl2ConfigFlags() {
#ifndef Q_OS_WIN
    QStringList c, l;
    const QString cstr = lmc_runShell("sdl2-config --cflags");
    const QString lstr = lmc_runShell("sdl2-config --libs");
    if (!cstr.isEmpty()) c = lmc_splitArgs(cstr);
    if (!lstr.isEmpty()) l = lmc_splitArgs(lstr);
    return {c, l};
#else
    return {};
#endif
}

static void lmc_addIfMissing(QStringList& dest, const QStringList& src) {
    for (const QString& s : src) if (!dest.contains(s)) dest << s;
}

struct LmcAutoNeed { bool sdl=false, sdl_ttf=false, sdl_image=false, glfw=false, sfml=false; };

static LmcAutoNeed lmc_scanIncludesForNeeds(const QStringList& sources) {
    LmcAutoNeed n;
    for (const QString& path : sources) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) continue;
        const QString text = QString::fromUtf8(f.readAll());
        if (text.contains("#include <SDL2/") || text.contains("#include <SDL.h>")) n.sdl = true;
        if (text.contains("#include <SDL_ttf.h>"))  { n.sdl = true; n.sdl_ttf = true; }
        if (text.contains("#include <SDL_image.h>")){ n.sdl = true; n.sdl_image = true; }
        if (text.contains("#include <GLFW/")) n.glfw = true;
        if (text.contains("#include <SFML/")) n.sfml = true;
    }
    return n;
}

static void lmc_addWindowsSDL2FromEnv(QStringList& incSwitches, QStringList& ldflags, QStringList& libs) {
#ifdef Q_OS_WIN
    const QString sdl2dir = qEnvironmentVariable("SDL2_DIR");
    if (!sdl2dir.isEmpty()) {
        const QString inc = sdl2dir + "/include";
        const QString lib = sdl2dir + "/lib";
        if (!lmc_containsSwitch(incSwitches, "-I" + inc)) incSwitches << ("-I" + inc);
        if (!ldflags.contains("-L" + lib)) ldflags << ("-L" + lib);
        if (!libs.contains("-lSDL2"))     libs << "-lSDL2";
        if (!libs.contains("-lSDL2main")) libs << "-lSDL2main";
    }
#endif
}

static void lmc_addMacBrewFallback(const QString& pkg, QStringList& incSwitches, QStringList& ldflags) {
#ifdef Q_OS_MAC
    const QString prefix = lmc_runShell("brew --prefix " + pkg);
    if (!prefix.isEmpty()) {
        const QString inc = prefix + "/include";
        const QString lib = prefix + "/lib";
        if (!lmc_containsSwitch(incSwitches, "-I" + inc)) incSwitches << ("-I" + inc);
        if (!ldflags.contains("-L" + lib)) ldflags << ("-L" + lib);
    }
#endif
}

// build | clean
void MainWindow::buildProject() {
    ui->outputBox->clear();

    QString out = ui->outputPathInput->text().trimmed();
    if (out.isEmpty()) { appendLog("❌ Please choose an output path.\n"); return; }
    out = targetPathWithExt(out);

    // get src files
    QStringList sources;
    for (int i = 0; i < ui->fileList->count(); ++i) {
        const QString path = ui->fileList->item(i)->text();
        if (path.endsWith(".c", Qt::CaseInsensitive) ||
            path.endsWith(".cc", Qt::CaseInsensitive) ||
            path.endsWith(".cpp", Qt::CaseInsensitive)) {
            sources << path;
        }
    }
    if (sources.isEmpty()) { appendLog("❌ Add at least one source file.\n"); return; }

    // check for multiple main() functions (mostly for me because i'm a doughnut)
    QStringList mainFiles;
    static const QRegularExpression kMainRegex(R"(\bint\s+main\s*\()");

    for (int i = 0; i < sources.size(); ++i) {
        const QString &src = sources.at(i);
        QFile f(src);
        if (f.open(QIODevice::ReadOnly)) {
            const QString content = QString::fromUtf8(f.readAll());
            if (kMainRegex.match(content).hasMatch()) {
                mainFiles << QFileInfo(src).fileName();
            }
        }
    }

    if (mainFiles.size() > 1) {
        appendLog("⚠️ Multiple main() functions found:\n");
        for (const QString &f : mainFiles) appendLog("   - " + f + "\n");
        appendLog("❌ Only one main() is allowed per program. Please deselect extra files.\n");
        return;
    }

    // flags from front-end UI
    auto parseBox = [&](QPlainTextEdit* box){ QStringList L; for (auto &line : box->toPlainText().split('\n')) { auto t=line.trimmed(); if(!t.isEmpty()) L<<t; } return L; };
    QStringList cxxLines = parseBox(ui->cxxFlagsEdit);
    QStringList incLines = parseBox(ui->includeDirsEdit);
    QStringList defLines = parseBox(ui->definesEdit);
    QStringList ldLines  = parseBox(ui->ldFlagsEdit);
    QStringList libLines = parseBox(ui->libsEdit);

    // split like shell, accept "VAR = ..." style
    auto stripMakeVar = [](const QString& line)->QString {
        int eq = line.indexOf('=');
        if (eq >= 0) return line.mid(eq + 1).trimmed();
        return line.trimmed();
    };
    auto linesToArgs = [&](const QStringList& lines)->QStringList {
        QStringList args;
        for (const QString& ln : lines) {
            const QString s = stripMakeVar(ln);
            if (!s.isEmpty()) args << lmc_splitArgs(s);
        }
        return args;
    };

    QStringList cxxflags = linesToArgs(cxxLines);
    QStringList incs     = linesToArgs(incLines);
    QStringList defs     = linesToArgs(defLines);
    QStringList ldflags  = linesToArgs(ldLines);
    QStringList libs     = linesToArgs(libLines);

    // normalize -I / -D for bare values
    QStringList incSwitches; for (auto &i : incs) incSwitches << (i.startsWith("-I") ? i : "-I"+i);
    QStringList defSwitches; for (auto &d : defs) defSwitches << (d.startsWith("-D") ? d : "-D"+d);

    // C++ standard
    const QString stdSel = ui->stdCombo->currentText();
    if (stdSel.startsWith("c++")) cxxflags << ("-std="+stdSel);

    // flag auto-detection logic
    const LmcAutoNeed need = lmc_scanIncludesForNeeds(sources);

    // sdl2 family
    if (need.sdl) {
        auto sdlCfg = lmc_sdl2ConfigFlags();
        lmc_addIfMissing(cxxflags, sdlCfg.first);
        lmc_addIfMissing(libs,    sdlCfg.second);

        auto sdlPkg = lmc_pkgConfigFlags("sdl2");
        lmc_addIfMissing(cxxflags, sdlPkg.first);
        lmc_addIfMissing(libs,    sdlPkg.second);

#ifdef Q_OS_MAC
        if (!lmc_containsSwitch(cxxflags, "-I/opt/homebrew/include") &&
            !lmc_containsSwitch(cxxflags, "-I/usr/local/include")) {
            lmc_addMacBrewFallback("sdl2", incSwitches, ldflags);
            if (!lmc_containsSwitch(incSwitches, "-I/opt/homebrew/include"))
                incSwitches << "-I/opt/homebrew/include";
        }
#endif
        if (need.sdl_ttf) {
            auto ttfPkg = lmc_pkgConfigFlags("SDL2_ttf");
            lmc_addIfMissing(cxxflags, ttfPkg.first);
            lmc_addIfMissing(libs,     ttfPkg.second);
            if (!libs.contains("-lSDL2_ttf")) libs << "-lSDL2_ttf";
#ifdef Q_OS_MAC
            lmc_addMacBrewFallback("sdl2_ttf", incSwitches, ldflags);
#endif
        }
        if (need.sdl_image) {
            auto imgPkg = lmc_pkgConfigFlags("SDL2_image");
            lmc_addIfMissing(cxxflags, imgPkg.first);
            lmc_addIfMissing(libs,     imgPkg.second);
            if (!libs.contains("-lSDL2_image")) libs << "-lSDL2_image";
#ifdef Q_OS_MAC
            lmc_addMacBrewFallback("sdl2_image", incSwitches, ldflags);
#endif
        }

        lmc_addWindowsSDL2FromEnv(incSwitches, ldflags, libs);

        // ensure link when flimsy auto-detect can't find pkg-config/sdl2-config
#ifdef Q_OS_MAC
        if (!ldflags.contains("-L/opt/homebrew/lib")) ldflags << "-L/opt/homebrew/lib";
#endif
        if (!libs.contains("-lSDL2")) libs << "-lSDL2";
    }

    // GLFW
    if (need.glfw) {
        auto P = lmc_pkgConfigFlags("glfw3");
        lmc_addIfMissing(cxxflags, P.first);
        lmc_addIfMissing(libs,     P.second);
#ifdef Q_OS_MAC
        lmc_addMacBrewFallback("glfw", incSwitches, ldflags);
#endif
        if (!libs.contains("-lglfw")) libs << "-lglfw";
    }

    // SFML
    if (need.sfml) {
        QStringList sfmlPkgs = {"sfml-graphics", "sfml-window", "sfml-system", "sfml-audio"};
        for (const QString& pkg : sfmlPkgs) {
            auto P = lmc_pkgConfigFlags(pkg);
            lmc_addIfMissing(cxxflags, P.first);
            lmc_addIfMissing(libs,     P.second);
        }
#ifdef Q_OS_MAC
        lmc_addMacBrewFallback("sfml", incSwitches, ldflags);
#endif
    }

    // compiler path
    QString compiler = compilerCmd();

    // Build one final command: clang++ srcs -o out [flags]
    QStringList args;
    args << sources << "-o" << out;
    args << cxxflags << incSwitches << defSwitches << ldflags << libs;

    appendLog("Command: " + compiler + " " + args.join(" ") + "\n");

    if (!runProcess(compiler, args)) {
        appendLog("❌ Build failed.\n");
        return;
    }

#if defined(Q_OS_UNIX) && !defined(Q_OS_WIN)
    QFile::setPermissions(out, QFile::permissions(out) | QFileDevice::ExeUser);
#endif

    // launch raw.executable (macOS/Linux: plain exec; MS Windows: .exe)
    {
        QFileInfo outputInfo(out);
        appendLog("Launching app...\n");
        if (!QProcess::startDetached("/usr/bin/open", {"-a", "Terminal", out}, QFileInfo(out).absolutePath())) {
            appendLog("⚠️ Could not launch automatically. Run manually: " + out + "\n");
        }
    }

    appendLog("✅ Build succeeded. Output: " + out + "\n");
}

void MainWindow::cleanBuild() {
    ui->outputBox->clear();

    QString out = ui->outputPathInput->text().trimmed();
    if (out.isEmpty()) {
        appendLog("ℹ️ Nothing to clean (no target set).\n");
        return;
    }

    // normalize same ruleset as build (no .out on macOS/Linux(Unix))
    out = targetPathWithExt(out);

    if (QFile::exists(out)) {
        if (QFile::remove(out)) appendLog("Removed: " + out + "\n");
        else appendLog("⚠️ Could not remove: " + out + "\n");
    } else {
        appendLog("ℹ️ No file at: " + out + "\n");
    }

    appendLog("Clean complete! \n");
}

