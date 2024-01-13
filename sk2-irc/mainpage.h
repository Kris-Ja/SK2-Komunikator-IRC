#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class mainpage;
}
QT_END_NAMESPACE

class mainpage : public QWidget
{
    Q_OBJECT

public:
    mainpage(QWidget *parent = nullptr);
    ~mainpage();

private slots:
    void on_sendButton_clicked();

private:
    Ui::mainpage *ui;
    void getUser();
};
#endif // MAINPAGE_H
