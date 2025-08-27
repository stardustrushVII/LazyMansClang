// (c) 2025 Stardust Softworks
#pragma once
#include <QMainWindow>
#include <QProcess>
#include <QVector>
#include <QFutureWatcher>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    void addFiles();
    void removeSelectedFiles();
    void clearFiles();
    void browseCompiler();
    void browseOutputPath();
    void checkCompilerArchitecture(const QString &compilerPath);


    void buildProject();
    void cleanBuild();


private:
    Ui::MainWindow *ui;


    QString compilerCmd() const; // resolve compiler path
    QString targetPathWithExt(QString) const; // add .exe/.out when missing
    QString buildDirForTarget(const QString &target) const;


    QStringList parseLines(const QString &text) const; // split by lines, trim, drop empties
    void appendLog(const QString &s);
    bool runProcess(const QString &program, const QStringList &args, QString *stdoutOut = nullptr, QString *stderrOut = nullptr);
};
