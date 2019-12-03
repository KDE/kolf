/* This file is part of the KDE libraries
 * Initial implementation:
 *     Copyright (c) 1997 Patrick Dowler <dowler@morgul.fsh.uvic.ca>
 * Rewritten and maintained by:
 *     Copyright (c) 2000 Dirk Mueller <mueller@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kolfNumInput.h"

#include <cmath>

#include <QApplication>
#include <QLabel>
#include <QResizeEvent>
#include <QSlider>
#include <QStyle>

#include <KConfigDialogManager>

static inline int calcDiffByTen(int x, int y)
{
    // calculate ( x - y ) / 10 without overflowing ints:
    return (x / 10) - (y / 10)  + (x % 10 - y % 10) / 10;
}

// ----------------------------------------------------------------------------

class kolfNumInputPrivate
{
public:
    kolfNumInputPrivate(kolfNumInput *q) :
        q(q),
        column1Width(0),
        column2Width(0),
        label(0),
        slider(0),
        labelAlignment(0)
    {
    }

    static kolfNumInputPrivate *get(const kolfNumInput *i)
    {
        return i->d;
    }

    kolfNumInput *q;
    int column1Width, column2Width;

    QLabel  *label;
    QSlider *slider;
    QSize    sliderSize, labelSize;

    Qt::Alignment labelAlignment;
};

#define K_USING_kolfNumInput_P(_d) kolfNumInputPrivate *_d = kolfNumInputPrivate::get(this)

kolfNumInput::kolfNumInput(QWidget *parent)
    : QWidget(parent), d(new kolfNumInputPrivate(this))
{
    setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    setFocusPolicy(Qt::StrongFocus);
    KConfigDialogManager::changedMap()->insert(QStringLiteral("kpIntNumInput"), SIGNAL(valueChanged(int)));
    KConfigDialogManager::changedMap()->insert(QStringLiteral("QSpinBox"), SIGNAL(valueChanged(int)));
    KConfigDialogManager::changedMap()->insert(QStringLiteral("kolfDoubleSpinBox"), SIGNAL(valueChanged(double)));
}

kolfNumInput::~kolfNumInput()
{
    delete d;
}

QSlider *kolfNumInput::slider() const
{
    return d->slider;
}

bool kolfNumInput::showSlider() const
{
    return d->slider;
}

void kolfNumInput::setLabel(const QString &label, Qt::Alignment a)
{
    if (label.isEmpty()) {
        delete d->label;
        d->label = 0;
        d->labelAlignment = {};
    } else {
        if (!d->label) {
            d->label = new QLabel(this);
        }
        d->label->setText(label);
        d->label->setObjectName(QStringLiteral("kolfNumInput::QLabel"));
        d->label->setAlignment(a);
        // if no vertical alignment set, use Top alignment
        if (!(a & (Qt::AlignTop | Qt::AlignBottom | Qt::AlignVCenter))) {
            a |= Qt::AlignTop;
        }
        d->labelAlignment = a;
    }

    layout();
}

QString kolfNumInput::label() const
{
    return d->label ? d->label->text() : QString();
}

void kolfNumInput::layout()
{
    // label sizeHint
    d->labelSize = (d->label ? d->label->sizeHint() : QSize(0, 0));

    if (d->label && (d->labelAlignment & Qt::AlignVCenter)) {
        d->column1Width = d->labelSize.width() + 4;
    } else {
        d->column1Width = 0;
    }

    // slider sizeHint
    d->sliderSize = (d->slider ? d->slider->sizeHint() : QSize(0, 0));

    doLayout();

}

QSize kolfNumInput::sizeHint() const
{
    return minimumSizeHint();
}

void kolfNumInput::setSteps(int minor, int major)
{
    if (d->slider) {
        d->slider->setSingleStep(minor);
        d->slider->setPageStep(major);
    }
}

// ----------------------------------------------------------------------------

class kolfDoubleNumInput::kolfDoubleNumInputPrivate
{
public:
    kolfDoubleNumInputPrivate()
        : spin(0) {}
    QDoubleSpinBox *spin;
    QSize editSize;
    QString specialValue;
};

kolfDoubleNumInput::kolfDoubleNumInput(QWidget *parent)
    : kolfNumInput(parent)
    , d(new kolfDoubleNumInputPrivate())

{
    initWidget(0.0, 0.0, 9999.0, 0.01, 2);
}

kolfDoubleNumInput::kolfDoubleNumInput(double lower, double upper, double value, QWidget *parent,
                                 double singleStep, int precision)
    : kolfNumInput(parent)
    , d(new kolfDoubleNumInputPrivate())
{
    initWidget(value, lower, upper, singleStep, precision);
}

kolfDoubleNumInput::~kolfDoubleNumInput()
{
    delete d;
}

QString kolfDoubleNumInput::specialValueText() const
{
    return d->specialValue;
}

void kolfDoubleNumInput::initWidget(double value, double lower, double upper,
                                 double singleStep, int precision)
{
    d->spin = new QDoubleSpinBox(this);
    d->spin->setRange(lower, upper);
    d->spin->setSingleStep(singleStep);
    d->spin->setValue(value);
    d->spin->setDecimals(precision);

    d->spin->setObjectName(QStringLiteral("kolfDoubleNumInput::QDoubleSpinBox"));
    setFocusProxy(d->spin);
    connect(d->spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &kolfDoubleNumInput::valueChanged);

    layout();
}

double kolfDoubleNumInput::mapSliderToSpin(int val) const
{
    K_USING_kolfNumInput_P(priv);

    // map [slidemin,slidemax] to [spinmin,spinmax]
    const double spinmin = d->spin->minimum();
    const double spinmax = d->spin->maximum();
    const double slidemin = priv->slider->minimum(); // cast int to double to avoid
    const double slidemax = priv->slider->maximum(); // overflow in rel denominator
    const double rel = (double(val) - slidemin) / (slidemax - slidemin);
    return spinmin + rel * (spinmax - spinmin);
}

void kolfDoubleNumInput::sliderMoved(int val)
{
    d->spin->setValue(mapSliderToSpin(val));
}

void kolfDoubleNumInput::spinBoxChanged(double val)
{
    K_USING_kolfNumInput_P(priv);

    const double spinmin = d->spin->minimum();
    const double spinmax = d->spin->maximum();
    const double slidemin = priv->slider->minimum(); // cast int to double to avoid
    const double slidemax = priv->slider->maximum(); // overflow in rel denominator

    const double rel = (val - spinmin) / (spinmax - spinmin);

    if (priv->slider) {
        priv->slider->blockSignals(true);
        priv->slider->setValue(qRound(slidemin + rel * (slidemax - slidemin)));
        priv->slider->blockSignals(false);
    }
}

QSize kolfDoubleNumInput::minimumSizeHint() const
{
    K_USING_kolfNumInput_P(priv);

    ensurePolished();

    int w;
    int h;

    h = qMax(d->editSize.height(), priv->sliderSize.height());

    // if in extra row, then count it here
    if (priv->label && (priv->labelAlignment & (Qt::AlignBottom | Qt::AlignTop))) {
        h += 4 + priv->labelSize.height();
    } else {
        // label is in the same row as the other widgets
        h = qMax(h, priv->labelSize.height() + 2);
    }

    const int spacingHint = style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing);
    w = priv->slider ? priv->slider->sizeHint().width() + spacingHint : 0;
    w += priv->column1Width + priv->column2Width;

    if (priv->labelAlignment & (Qt::AlignTop | Qt::AlignBottom)) {
        w = qMax(w, priv->labelSize.width() + 4);
    }

    return QSize(w, h);
}

void kolfDoubleNumInput::resizeEvent(QResizeEvent *e)
{
    K_USING_kolfNumInput_P(priv);

    int w = priv->column1Width;
    int h = 0;
    const int spacingHint = style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing);

    if (priv->label && (priv->labelAlignment & Qt::AlignTop)) {
        priv->label->setGeometry(0, 0, e->size().width(), priv->labelSize.height());
        h += priv->labelSize.height() + 4;
    }

    if (priv->label && (priv->labelAlignment & Qt::AlignVCenter)) {
        priv->label->setGeometry(0, 0, w, d->editSize.height());
    }

    if (qApp->layoutDirection() == Qt::RightToLeft) {
        d->spin->setGeometry(w, h, priv->slider ? priv->column2Width
                             : e->size().width() - w, d->editSize.height());
        w += priv->column2Width + spacingHint;

        if (priv->slider) {
            priv->slider->setGeometry(w, h, e->size().width() - w, d->editSize.height() + spacingHint);
        }
    } else if (priv->slider) {
        priv->slider->setGeometry(w, h, e->size().width() -
                                  (priv->column1Width + priv->column2Width + spacingHint),
                                  d->editSize.height() + spacingHint);
        d->spin->setGeometry(w + priv->slider->width() + spacingHint, h,
                             priv->column2Width, d->editSize.height());
    } else {
        d->spin->setGeometry(w, h, e->size().width() - w, d->editSize.height());
    }

    h += d->editSize.height() + 2;

    if (priv->label && (priv->labelAlignment & Qt::AlignBottom)) {
        priv->label->setGeometry(0, h, priv->labelSize.width(), priv->labelSize.height());
    }
}

void kolfDoubleNumInput::doLayout()
{
    K_USING_kolfNumInput_P(priv);

    d->editSize = d->spin->sizeHint();
    priv->column2Width = d->editSize.width();
}

void kolfDoubleNumInput::setValue(double val)
{
    d->spin->setValue(val);
}

void kolfDoubleNumInput::setRange(double lower, double upper, double singleStep)
{
    K_USING_kolfNumInput_P(priv);

    QDoubleSpinBox *spin = d->spin;

    d->spin->setRange(lower, upper);
    d->spin->setSingleStep(singleStep);

    const double range = spin->maximum() - spin->minimum();
    const double steps = range * pow(10.0, spin->decimals());
    if (!priv->slider) {
        priv->slider = new QSlider(Qt::Horizontal, this);
        priv->slider->setTickPosition(QSlider::TicksBelow);
        // feedback line: when one moves, the other moves, too:
        connect(priv->slider, &QSlider::valueChanged,
                this, &kolfDoubleNumInput::sliderMoved);
        layout();
    }
    if (steps > 1000 ) {
        priv->slider->setRange(0, 1000);
        priv->slider->setSingleStep(1);
        priv->slider->setPageStep(50);
    } else {
        const int singleSteps = qRound(steps);
        priv->slider->setRange(0, singleSteps);
        priv->slider->setSingleStep(1);
        const int pageSteps = qBound(1, singleSteps / 20, 10);
        priv->slider->setPageStep(pageSteps);
    }
    spinBoxChanged(spin->value());
    connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &kolfDoubleNumInput::spinBoxChanged);

    layout();
}

void kolfDoubleNumInput::setMinimum(double min)
{
    setRange(min, maximum(), d->spin->singleStep());
}

double kolfDoubleNumInput::minimum() const
{
    return d->spin->minimum();
}

void kolfDoubleNumInput::setMaximum(double max)
{
    setRange(minimum(), max, d->spin->singleStep());
}

double kolfDoubleNumInput::maximum() const
{
    return d->spin->maximum();
}

double kolfDoubleNumInput::singleStep() const
{
    return d->spin->singleStep();
}

void kolfDoubleNumInput::setSingleStep(double singleStep)
{
    d->spin->setSingleStep(singleStep);
}

double kolfDoubleNumInput::value() const
{
    return d->spin->value();
}

QString kolfDoubleNumInput::suffix() const
{
    return d->spin->suffix();
}

void kolfDoubleNumInput::setSuffix(const QString &suffix)
{
    d->spin->setSuffix(suffix);

    layout();
}

void kolfDoubleNumInput::setDecimals(int decimals)
{
    d->spin->setDecimals(decimals);

    layout();
}

int kolfDoubleNumInput::decimals() const
{
    return d->spin->decimals();
}

void kolfDoubleNumInput::setSpecialValueText(const QString &text)
{
    d->spin->setSpecialValueText(text);

    layout();
}

void kolfDoubleNumInput::setLabel(const QString &label, Qt::Alignment a)
{
    K_USING_kolfNumInput_P(priv);

    kolfNumInput::setLabel(label, a);

    if (priv->label) {
        priv->label->setBuddy(d->spin);
    }
}

#include "moc_kolfNumInput.cpp"
