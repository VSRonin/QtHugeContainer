#ifndef bench_hugemap_h__
#define bench_hugemap_h__

#include <QObject>
class bench_hugemap : public QObject
{
    Q_OBJECT
private slots:
    void benchHugeInsert_data();
    void benchHugeReadKey_data() { benchHugeInsert_data(); }
    void benchHugeReadIter_data() { benchHugeInsert_data(); }
    void benchHugeReadKeyReverse_data() { benchHugeInsert_data(); }
    void benchHugeReadIterReverse_data() { benchHugeInsert_data(); }

    void benchHugeInsert();
    void benchHugeReadKey();
    void benchHugeReadIter();
    void benchHugeReadKeyReverse();
    void benchHugeReadIterReverse();

    void benchQtInsert_data();
    void benchQtReadKey_data() { benchQtInsert_data(); }
    void benchQtReadIter_data() { benchQtInsert_data(); }
    void benchStdInsert_data() { benchQtInsert_data(); }
    void benchStdReadKey_data() { benchQtInsert_data(); }
    void benchStdReadIter_data() { benchQtInsert_data(); }

    void benchQtInsert();
    void benchQtReadKey();
    void benchQtReadIter();
    void benchStdInsert();
    void benchStdReadKey();
    void benchStdReadIter();
};
#endif // bench_hugemap_h__