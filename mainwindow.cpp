#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QDate>
#include <QMessageBox>

//Global Variable
int currentYear = QDate::currentDate().year();

// Per-column regex patterns used by both the form validators and the delegate
static const QString AUTHOR_REGEX = "^[A-Za-z]+(?: [A-Za-z]+)*(?:-[A-Za-z]+)?(?: [A-Za-z]+(?:-[A-Za-z]+)?)*$";
static const QString TITLE_REGEX  = "^([A-Za-z]+|[0-9]+)(?:\\s+([A-Za-z]+|[0-9]+))*$";
static const QString PAGES_REGEX  = "^\\d+\\s*-\\s*\\d+$";

// ── ValidationDelegate ──────────────────────────────────────────────────────

ValidationDelegate::ValidationDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {}

QWidget* ValidationDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    // Year, Volume and Issue columns use the default spinbox editor
    int col = index.column();
    if (col == MainWindow::YEAR || col == MainWindow::VOLUME || col == MainWindow::ISSUE) {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

    // For text columns attach the appropriate regex validator
    QString pattern;
    switch (col) {
    case MainWindow::AUTHOR:  pattern = AUTHOR_REGEX; break;
    case MainWindow::TITLE:   pattern = TITLE_REGEX;  break;
    case MainWindow::JOURNAL: pattern = TITLE_REGEX;  break;
    case MainWindow::PAGES:   pattern = PAGES_REGEX;  break;
    default: return QStyledItemDelegate::createEditor(parent, option, index);
    }

    QLineEdit *editor = new QLineEdit(parent);
    editor->setValidator(new QRegularExpressionValidator(QRegularExpression(pattern), editor));
    return editor;
}

void ValidationDelegate::setModelData(QWidget *editor,
                                      QAbstractItemModel *model,
                                      const QModelIndex &index) const
{
    int col = index.column();

    // Numeric columns — delegate to default behaviour; year is handled separately
    // in MainWindow's dataChanged slot
    if (col == MainWindow::VOLUME || col == MainWindow::ISSUE) {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }
    if (col == MainWindow::YEAR) {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    // Text columns — validate the full string before committing
    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
    if (!lineEdit) {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    QString text = lineEdit->text().trimmed();
    QString pattern;
    switch (col) {
    case MainWindow::AUTHOR:  pattern = AUTHOR_REGEX; break;
    case MainWindow::TITLE:   pattern = TITLE_REGEX;  break;
    case MainWindow::JOURNAL: pattern = TITLE_REGEX;  break;
    case MainWindow::PAGES:   pattern = PAGES_REGEX;  break;
    default: break;
    }

    if (!pattern.isEmpty()) {
        QRegularExpression rx(pattern);
        if (!rx.match(text).hasMatch()) {
            // Invalid — warn and leave the model unchanged (reverts to previous value)
            QMessageBox::warning(nullptr, "Validation Error",
                                 QString("The value \"%1\" is not valid for this field.\n"
                                         "The original value has been restored.").arg(text));
            return; // Do NOT call setModelData — model keeps previous value
        }
    }

    model->setData(index, text, Qt::EditRole);
    model->setData(index, text, Qt::DisplayRole);
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Create model: 0 rows, COLUMN_COUNT columns
    model = new QStandardItemModel(0,COLUMN_COUNT,this);

    //Column Headers
    QStringList headers = {"Author","Year","Title","Journal","Volume","Issue","Pages"};
    model->setHorizontalHeaderLabels(headers);

    //Populate Model
    addSampleData();

    // Create and configure the proxy model for sorting
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setSortRole(Qt::EditRole); // Sort by the actual data, not display text
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive); // For non-case sensitive sort

    proxyModel->setFilterKeyColumn(AUTHOR); // Will be set by combo box

    ui->filterComboBox->addItems({"Author", "Year", "Title", "Journal", "Volume", "Issue"});

    //Connect model to view
    ui->tableView->setModel(proxyModel);
    ui->tableView->setSortingEnabled(true); // Enable sorting by clicking headers
    ui->tableView->setEditTriggers(QAbstractItemView::DoubleClicked); // Enable editing

    //Min/Max values
    ui->yearSpinBox->setMinimum(1900);
    ui->yearSpinBox->setMaximum(currentYear);
    ui->volumeSpinBox->setMinimum(1);
    ui->issueSpinBox->setMinimum(1);

    // Connect model dataChanged signal to update highlighting when edited
    connect(model, &QStandardItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
        // Check if year column was changed
        if (topLeft.column() <= YEAR && bottomRight.column() >= YEAR) {
            for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
                QStandardItem* yearItem = model->item(row, YEAR);
                if (yearItem) {
                    int year = yearItem->data(Qt::EditRole).toInt();
                    // Validate and correct if necessary
                    if (!isValidYear(year)) {
                        yearItem->setData(currentYear, Qt::EditRole);
                        yearItem->setData(currentYear, Qt::DisplayRole);
                        yearItem->setData(currentYear, Qt::UserRole);
                    }
                }
                highlightRow(row);
            }
        }
    });

    // Apply initial highlighting
    highlightAllRows();

    // Attach input validators to form fields
    setValidators();

    // Attach validation delegate to the table view
    setDelegate();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addRow(const QString& author, int year, const QString& title, const QString& journal, int volume, int issue, const QString& pages)
{
    QList<QStandardItem*> items;

    // Text items
    items.append(new QStandardItem(author));

    // Year - store as integer for sorting, display as string
    QStandardItem* yearItem = new QStandardItem();
    yearItem->setData(year, Qt::DisplayRole);  // Shows as number
    yearItem->setData(year, Qt::UserRole);     // For sorting numerically
    yearItem->setData(year, Qt::EditRole);     // For Editing
    items.append(yearItem);

    items.append(new QStandardItem(title));
    items.append(new QStandardItem(journal));

    // Volume - store as integer
    QStandardItem* volumeItem = new QStandardItem();
    volumeItem->setData(volume, Qt::DisplayRole);
    volumeItem->setData(volume, Qt::UserRole);
    volumeItem->setData(volume, Qt::EditRole);
    items.append(volumeItem);

    // Issue - store as integer
    QStandardItem* issueItem = new QStandardItem();
    issueItem->setData(issue, Qt::DisplayRole);
    issueItem->setData(issue, Qt::UserRole);
    issueItem->setData(issue, Qt::EditRole);
    items.append(issueItem);

    items.append(new QStandardItem(pages));

    model->appendRow(items);
    //Highlight added row
    highlightRow(model->rowCount() - 1);
}

void MainWindow::fullReset()
{
    ui->authorEdit->clear();
    ui->yearSpinBox->setValue(ui->yearSpinBox->minimum());
    ui->titleEdit->clear();
    ui->journalEdit->clear();
    ui->volumeSpinBox->setValue(ui->volumeSpinBox->minimum());
    ui->issueSpinBox->setValue(ui->issueSpinBox->minimum());
    ui->pagesEdit->clear();
    ui->filterEdit->clear();

    applyFilter();
}

void MainWindow::addSampleData()
{
    addRow("Johnson, M.", 2023, "Quantum Computing Algorithms", "Nature Computing", 45, 3, "234-248");
    addRow("Chen, W.", 2022, "Machine Learning in Healthcare", "IEEE Transactions", 28, 4, "112-125");
    addRow("Patel, R.", 2024, "Blockchain Security Protocols", "Journal of Cryptography", 12, 1, "78-92");
    addRow("Garcia, L.", 2023, "Neural Network Architecture", "AI Research", 33, 2, "445-460");
    addRow("Kim, S.", 2022, "Cloud Computing Optimization", "Distributed Systems", 19, 5, "567-582");
    addRow("Williams, A.", 2023, "COVID-19 Long-term Effects", "The Lancet", 401, 15, "1123-1135");
    addRow("Martinez, C.", 2024, "CRISPR Gene Editing", "Nature Genetics", 56, 2, "89-104");
    addRow("Thompson, R.", 2016, "Alzheimer's Early Detection", "Neurology Today", 18, 7, "678-692");
    addRow("Anderson, K.", 2014, "Cancer Immunotherapy", "Oncology Reports", 44, 3, "234-249");
    addRow("Liu, Y.", 2024, "Stem Cell Research Progress", "Cell Biology", 67, 1, "45-59");
    addRow("Hawking, S.", 1990, "Black Hole Information Paradox", "Astrophysical Journal", 925, 2, "345-360");
    addRow("Rodriguez, M.", 2012, "Quantum Entanglement Applications", "Physics Review", 88, 4, "567-582");
    addRow("Taylor, J.", 1982, "Dark Matter Detection Methods", "Physical Review D", 109, 6, "890-905");
    addRow("Brown, D.", 2006, "Exoplanet Atmosphere Analysis", "Astronomy & Astrophysics", 672, 3, "123-138");
    addRow("Zhang, L.", 2009, "Gravitational Wave Observations", "Nature Physics", 18, 8, "901-916");
}

void MainWindow::applyFilter()
{
    //Find
    QString filterText = ui->filterEdit->text();
    int columnIndex = ui->filterComboBox->currentIndex();

    // Map combo box index to actual column (skip page number which is column 6)
    // Combo box indexes match model indexes up to column 5 (Issue)
    if (columnIndex >= 0 && columnIndex <= 5) {
        proxyModel->setFilterKeyColumn(columnIndex);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterWildcard(filterText);
    }
}

bool MainWindow::isValidYear(int year)
{
    return (year > 1899 && year <= currentYear);
}

QColor MainWindow::getHighlightColorForYear(int year)
{
    int refAge = currentYear - year;
    if (refAge >= 10) {
        return QColor(255,0,0); //Red
    } else if (refAge <= 5) {
        return QColor(0,170,0); //Darkish Green
    }
    return QColor(255,255,255); //White
}

void MainWindow::highlightRow(int row)
{
    // Get year from the model (column 1)
    QStandardItem* yearItem = model->item(row, YEAR);
    if (!yearItem) return;

    int year = yearItem->data(Qt::EditRole).toInt();
    QColor color = getHighlightColorForYear(year);

    // Apply color to all columns in the row
    for (int col = 0; col < COLUMN_COUNT; ++col) {
        QStandardItem* item = model->item(row, col);
        if (item) {
            item->setBackground(color);
        }
    }
}

void MainWindow::highlightAllRows()
{
    for (int row = 0; row < model->rowCount(); ++row) {
        highlightRow(row);
    }
}

void MainWindow::setValidators()
{
    authorValidator = new QRegularExpressionValidator(QRegularExpression(AUTHOR_REGEX), this);
    titleValidator  = new QRegularExpressionValidator(QRegularExpression(TITLE_REGEX),  this);
    pagesValidator  = new QRegularExpressionValidator(QRegularExpression(PAGES_REGEX),  this);

    ui->authorEdit->setValidator(authorValidator);
    ui->titleEdit->setValidator(titleValidator);
    ui->journalEdit->setValidator(titleValidator);
    ui->pagesEdit->setValidator(pagesValidator);
}

void MainWindow::setDelegate()
{
    validationDelegate = new ValidationDelegate(this);
    ui->tableView->setItemDelegate(validationDelegate);
}



/////////////////////////Buttons
void MainWindow::on_btnAdd_clicked()
{
    //Add
    QString author = ui->authorEdit->text().trimmed();
    int year = ui->yearSpinBox->value(); //validate year
    QString title = ui->titleEdit->text().trimmed();
    QString journal = ui->journalEdit->text().trimmed();
    int volume = ui->volumeSpinBox->value();
    int issue = ui->issueSpinBox->value();
    QString pages = ui->pagesEdit->text().trimmed();

    //Validation check
    if (author.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Author cannot be empty");
        return;
    }
    if (!isValidYear(year)) {
        QMessageBox::warning(this, "Validation Error", "Year not valid");
        return;
    }
    if (title.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Title cannot be empty");
        return;
    }
    if (journal.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Journal cannot be empty");
        return;
    }
    if (volume < 1) {
        QMessageBox::warning(this, "Validation Error", "Volume cannot be less than 1");
        return;
    }
    if (issue < 1) {
        QMessageBox::warning(this, "Validation Error", "Issue cannot be less than 1");
        return;
    }
    if (pages.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Pages cannot be empty");
        return;
    }

    addRow(author,year,title,journal,volume,issue,pages);
}


void MainWindow::on_btnRemove_clicked()
{
    //Remove
    if (!ui->tableView->currentIndex().isValid()) {
        QMessageBox::warning(this, "No Selection", "Please select a row to remove");
        return;
    }
    proxyModel->removeRow(ui->tableView->currentIndex().row());
}


void MainWindow::on_btnFind_clicked()
{
    if(ui->filterEdit->text().isEmpty()){
        QMessageBox::warning(this,"Empty Search", "Please enter search criteria");
        return;
    }
    applyFilter();
}


void MainWindow::on_btnReset_clicked()
{
    //Reset
    fullReset();
}
