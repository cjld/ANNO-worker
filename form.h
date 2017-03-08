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
    int cdis;
    void paintEvent(QPaintEvent *);
    void colorDistanceChange(int v) {cdis = v;update();}

private:
};

#endif // FORM_H
