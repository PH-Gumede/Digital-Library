#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QRegularExpressionValidator>
#include <QStyledItemDelegate>
#include <QMessageBox>
#include <QLineEdit>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// Custom delegate to enforce per-column validation when editing cells in the view
class ValidationDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ValidationDelegate(QObject *parent = nullptr);

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum Columns {
        AUTHOR = 0,
        YEAR,
        TITLE,
        JOURNAL,
        VOLUME,
        ISSUE,
        PAGES,
        COLUMN_COUNT
    };



private slots:
    void on_btnAdd_clicked();

    void on_btnRemove_clicked();

    void on_btnFind_clicked();

    void on_btnReset_clicked();

private:
    Ui::MainWindow *ui;
    QStandardItemModel *model;
    QSortFilterProxyModel *proxyModel;
    QRegularExpressionValidator *authorValidator;
    QRegularExpressionValidator *titleValidator;
    QRegularExpressionValidator *pagesValidator;
    ValidationDelegate *validationDelegate;

    //Clear
    void fullReset();

    //Create Sample Data
    void addSampleData();

    //Function to add a new row
    void addRow(const QString& author, int year, const QString& title, const QString& journal, int volume, int issue, const QString& pages);

    //Apply filter
    void applyFilter();

    //Vaildate Year
    bool isValidYear(int year);

    //Create color for highlight
    QColor getHighlightColorForYear(int year);

    //Highlight 1 row
    void highlightRow(int row);

    //Highlight all rows
    void highlightAllRows();

    //Set Validators
    void setValidators();

    //Set Delegate
    void setDelegate();
};
#endif // MAINWINDOW_H
