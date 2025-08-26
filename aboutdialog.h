// (c) 2025 Stardust Softworks
#pragma once
#include <QDialog>

class QLabel;
class QTextBrowser;
class QPushButton;

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);


    void setCreditsHtml(const QString& html);
    void setExtraBuildLine(const QString& line);       // some extra idk

private:
    QString buildInfo() const;                         // qt/compiler/aarch/build time
    void copyAllToClipboard() const;

    QLabel* logoLabel{};
    QLabel* titleLabel{};
    QLabel* subtitleLabel{};
    QTextBrowser* credits{};
    QPushButton* copyBtn{};
    QPushButton* closeBtn{};
};
