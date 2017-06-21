/****************************************************************************
** Based on a modified version of tst_HugeMap
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
**
****************************************************************************/

#ifndef tst_hugemap_h__
#define tst_hugemap_h__

#include "hugecontainer.h"
#include <QtTest/QtTest>
#include <QDebug>
using namespace HugeContainers;

class tst_HugeMap : public QObject
{
    Q_OBJECT
private slots:
    void testConstructor();
    void testConstructor_data();
    
};

#endif // tst_hugemap_h__