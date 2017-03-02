#ifndef FORM_H
#define FORM_H

#include <QWidget>

namespace Ui {
class Form;
}

class MyWindow;

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = 0);
    ~Form();
    Ui::Form *ui;
    MyWindow *window;
    void paintEvent(QPaintEvent *);

private:
};

#endif // FORM_H
