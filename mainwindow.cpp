// (C) 2025 Stardust Softworks
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QProcess>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QFileDevice>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("LazyMansClang");
    this->setWindowIcon(QIcon(":/icons/128x128.png"));

    connect(ui->browseButton, &QPushButton::clicked, this, &MainWindow::browseFile1);
    connect(ui->browseButton2, &QPushButton::clicked, this, &MainWindow::browseFile2);
    connect(ui->compileButton, &QPushButton::clicked, this, &MainWindow::compileCode);
    connect(ui->browseOutputPath, &QPushButton::clicked, this, &MainWindow::browseOutputPath);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::browseFile1() {
    QString file = QFileDialog::getOpenFileName(this, "Select File", "", "C++ Files (*.cpp)");
    if (!file.isEmpty()) {
        ui->fileInput1->setText(file);
    }
}

void MainWindow::browseFile2() {
    QString file = QFileDialog::getOpenFileName(this, "Select File", "", "C++ Files (*.cpp)");
    if (!file.isEmpty()) {
        ui->fileInput2->setText(file);
    }
}

void MainWindow::browseOutputPath() {
    QString file = QFileDialog::getSaveFileName(this, "Choose Output FIle", "", "Executables(*.exe *.out);;All Files (*)");
    if (!file.isEmpty()) {
        ui->outputPathInput->setText(file);
    }
}

void MainWindow::compileCode() {
    QString file1 = ui->fileInput1->text();
    QString file2 = ui->fileInput2->text();

    if (file1.isEmpty() || file2.isEmpty()) {
        ui->outputBox->setText("❌ Error: Please select two .cpp files.");
        return;
    }

    QString outputFile = ui->outputPathInput->text();
    if (outputFile.isEmpty()) {
        ui->outputBox->setText("❌ Error: Please select an output path.");
        return;
    }

    // Append appropriate extension
#ifdef Q_OS_WIN
    if (!outputFile.endsWith(".exe", Qt::CaseInsensitive)) {
        outputFile += ".exe";
    }
#else
    if (!outputFile.endsWith(".out", Qt::CaseInsensitive)) {
        outputFile += ".out";
    }
#endif

    // Determine the correct compiler executable path
    QString compilerCmd;
#ifdef Q_OS_MAC
    compilerCmd = "/usr/bin/clang++";
#elif defined(Q_OS_WIN)
    compilerCmd = "clang++.exe";
#else
    compilerCmd = "clang++"; // Fallback for other Unix
#endif

    QStringList args = {
        QDir::toNativeSeparators(file1),
        QDir::toNativeSeparators(file2),
        "-o",
        QDir::toNativeSeparators(outputFile)
    };

    QProcess compiler;
    compiler.start(compilerCmd, args);
    compiler.waitForFinished();

    if (compiler.error() != QProcess::UnknownError) {
        QString processError = "QProcess error: " + compiler.errorString();
        ui->outputBox->setText("❌ QProcess Failed\n\n" + processError);
        return;
    }

    QString result = compiler.readAllStandardOutput();
    QString error = compiler.readAllStandardError();

    // Set Unix execute permissions on successful build (macOS/Linux)
#if defined(Q_OS_UNIX) && !defined(Q_OS_WIN)
    QFile outputCheck(outputFile);
    if (outputCheck.exists()) {
        QFile::setPermissions(outputFile, QFile::permissions(outputFile) | QFileDevice::ExeUser);
    }
#endif

    if (!error.isEmpty()) {
        ui->outputBox->setText("❌ Compilation Failed\n\nSTDERR:\n" + error + "\nSTDOUT:\n" + result);
    } else {
        ui->outputBox->setText("✅ Compilation Succeeded\n\nOutput path:\n" + outputFile + "\n\nSTDOUT:\n" + result);
    }

#if defined(Q_OS_MAC)
    // create launch.command on successful compile
    QFileInfo outputInfo(outputFile);
    QString wrapperPath = outputInfo.absolutePath() + "/launch.command";

    QFile launcher(wrapperPath);
    if (launcher.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&launcher);
        out << "#!/bin/bash\n";
        out << "cd \"$(dirname \"$0\")\"\n";
        out << "chmod +x './" << outputInfo.fileName() << "'\n";
        out << "./'" << outputInfo.fileName() << "'\n";
        launcher.close();

        // make launch.command executable
        QFile::setPermissions(wrapperPath, QFile::permissions(wrapperPath) | QFileDevice::ExeUser);
        QProcess::startDetached("open", QStringList() << wrapperPath);

        result += "\n\n Launch script created:\n" + wrapperPath;
    }
#endif

}
