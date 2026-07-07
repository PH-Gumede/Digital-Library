# Digital Library — Journal Reference Manager

A C++/Qt6 desktop application for cataloguing, sorting, filtering, and validating academic journal article references, built around Qt's Model/View architecture rather than manually-managed widgets.

![C++](https://img.shields.io/badge/C++-17-00599C?style=flat&logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?style=flat&logo=qt&logoColor=white)
![CMake](https://img.shields.io/badge/build-CMake-064F8C?style=flat&logo=cmake&logoColor=white)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

---

## Overview

> **Note on naming:** this repository is named "Digital Library," but the application it contains is more precisely a **journal article / citation reference manager** — every record has an Author, Year, Title, Journal, Volume, Issue, and Page range, not general library items like books or media. This README describes what the code actually does; see **Recommended Improvements** for a note on the naming.

- **What it does** — lets a user add, remove, search, and sort journal article references in a table, with regex-validated input fields and colour-coded rows indicating how recent each reference is.
- **Who it's for** — reviewers assessing proficiency with Qt's Model/View framework, input validation, and sortable/filterable table UIs — patterns directly transferable to real-world data-management tools.
- **Why it exists** — built as a university assignment (COS3711) to demonstrate `QStandardItemModel`, `QSortFilterProxyModel`, `QRegularExpressionValidator`, and a custom `QStyledItemDelegate` working together in one application.
- **The problem it solves** — shows a working, validated, sortable reference table — the same underlying pattern used in real citation managers like Zotero or Mendeley, implemented from first principles.

## Features

- 📚 **Journal reference table** — Author, Year, Title, Journal, Volume, Issue, and Pages columns, pre-populated with 15 realistic sample references spanning multiple academic fields.
- 🔃 **Click-to-sort columns** — clicking any column header sorts the table by that field, using the underlying data (not just the displayed text).
- 🔍 **Column-targeted wildcard search** — filter the table by Author, Year, Title, Journal, Volume, or Issue via a dropdown plus a search box.
- ✅ **Regex-validated input** — Author, Title, Journal, and Pages fields reject invalid input as it's typed, both in the entry form *and* when editing a cell directly in the table.
- 🎨 **Age-based row highlighting** — rows automatically colour-code by how recent the reference is (red for old, green for recent).
- ➕➖ **Add / Remove rows** — add a new validated reference or remove the currently selected row.
- 🧽 **Reset** — clears all form fields and the active filter in one action.

## Screenshots / Demo

No screenshots are currently included in this repository. See the **Assets Checklist** for what to add — at minimum, a screenshot of the populated table showing the red/green row highlighting, and one showing the validation warning dialog that appears when invalid input is entered, since validation is one of this project's core demonstrated skills.

## Live Demo

Not applicable — this is a compiled desktop application, not a web deployment. See **Installation** below to build and run it locally.

## Tech Stack

| Category | Technology |
|---|---|
| **Language** | C++17 |
| **GUI Framework** | Qt 6 (Widgets module) |
| **Data Layer** | `QStandardItemModel` + `QSortFilterProxyModel` (Model/View architecture) |
| **Validation** | `QRegularExpressionValidator`, custom `QStyledItemDelegate` |
| **Build System** | CMake (3.16+) |

## Project Structure

```
Digital-Library/
├── CMakeLists.txt          # Build configuration
├── main.cpp                  # Application entry point
└── mainwindow.{h,cpp,ui}      # All application logic:
                                #  - QStandardItemModel setup & sample data
                                #  - QSortFilterProxyModel wiring for sort/filter
                                #  - ValidationDelegate (custom QStyledItemDelegate)
                                #  - Row highlighting by publication year
                                #  - Add / Remove / Find / Reset button handlers
```

Unlike the sibling **Shape Drawing App**, this project keeps all logic in a single `MainWindow` class (plus one small delegate class) rather than splitting the domain model into its own files — appropriate for its smaller scope, though see **Future Improvements** for how this could be factored out as the project grows.

## Installation

**Prerequisites:** Qt 6 (Widgets module), CMake 3.16+, and a C++17-capable compiler.

```bash
git clone https://github.com/PH-Gumede/Digital-Library.git
cd Digital-Library

mkdir build && cd build
cmake ..
cmake --build .
```

Or open `CMakeLists.txt` directly in Qt Creator and build using the IDE.

## Usage

1. The table launches pre-populated with 15 sample journal references, already sorted and colour-coded by age.
2. Fill in the Author, Year, Title, Journal, Volume, Issue, and Pages fields and click **Add** — invalid input (empty required fields, out-of-range years, malformed page ranges) is rejected with a warning dialog.
3. Click any column header to sort the table by that column.
4. Choose a column from the filter dropdown, type a search term, and click **Find** to filter the visible rows by wildcard match.
5. Double-click any cell to edit it in place — the same validation rules apply to inline edits as to the entry form.
6. Select a row and click **Remove** to delete it, or click **Reset** to clear all form fields and the active filter.

## Key Technical Concepts

**Model/View separation via a proxy model.** Rather than filtering or sorting the underlying data directly, the app layers a `QSortFilterProxyModel` on top of the real `QStandardItemModel` and points the table view at the *proxy*:

```cpp
proxyModel = new QSortFilterProxyModel(this);
proxyModel->setSourceModel(model);
proxyModel->setSortRole(Qt::EditRole);       // sort by real data, not display text
proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

ui->tableView->setModel(proxyModel);
```

This means sorting and filtering never touch or reorder the actual source data — they only change what the proxy *presents* to the view, which is the correct, idiomatic Qt pattern for this kind of table.

**Dual-layer validation with a shared source of truth.** The same regex patterns are used to validate both the entry form (via `QRegularExpressionValidator` on each `QLineEdit`) *and* in-place table cell edits (via a custom delegate), defined once at file scope so they can never drift out of sync:

```cpp
static const QString AUTHOR_REGEX = "^[A-Za-z]+(?: [A-Za-z]+)*(?:-[A-Za-z]+)?...$";
static const QString PAGES_REGEX  = "^\\d+\\s*-\\s*\\d+$";
```

**A custom `QStyledItemDelegate` for in-table validation.** Qt's default table editing simply commits whatever text is typed. `ValidationDelegate` overrides `createEditor()` to attach the correct regex validator per column, and overrides `setModelData()` to re-check the full string before committing:

```cpp
QString text = lineEdit->text().trimmed();
if (!rx.match(text).hasMatch()) {
    QMessageBox::warning(nullptr, "Validation Error", /* ... */);
    return; // model is left unchanged — edit is silently rejected
}
model->setData(index, text, Qt::EditRole);
```

This is meaningfully more advanced than form-only validation: it guarantees data integrity no matter *how* a value enters the table, not just through the Add button.

**Self-correcting data via signal/slot monitoring.** Rather than only validating the Year at entry time, the app also listens to the model's own `dataChanged` signal and re-validates any edited year on the fly, silently resetting it to the current year if an in-place edit makes it invalid — a defensive pattern that keeps the dataset consistent regardless of how it's modified.

## Challenges & Solutions

- **Challenge:** ensuring a reference's colour-coding stays correct even when its year is edited directly in the table, not just when added through the form.
  **Solution:** connecting to the model's `dataChanged` signal means *any* edit to the Year column — from any source — triggers both validation and a re-highlight of that row, rather than only handling the "Add" button's code path.

- **Challenge:** validating table cell edits with the same rules as the entry form, without duplicating the regex logic.
  **Solution:** the regex patterns are declared once at file scope and referenced by both the form validators (`setValidators()`) and the delegate's `createEditor()`/`setModelData()` methods.

## Known Limitations

In the interest of being upfront about the current state of the project:

- **There is no data persistence.** All 15 sample references are generated in memory on startup (`addSampleData()`), and any references you add are lost the moment you close the application — nothing is saved to disk. If you need the desktop-app pattern for XML file save/load, see the sibling **Shape Drawing App** repository, which implements that.
- **The age-based highlighting has a small gap.** A reference exactly 6–9 years old matches neither the "≥10 years → red" nor the "≤5 years → green" condition, so it renders with no highlight at all rather than an intermediate colour.
- **The project name doesn't fully match its scope.** "Digital Library" suggests a general media/book catalogue; the actual data model (Author/Year/Title/Journal/Volume/Issue/Pages) is specifically a citation/reference manager for journal articles.
- The CMake project target embeds a student number (`COS3711A1-Q3-24660892`), which is fine for an assignment submission but worth reconsidering for a public portfolio repository.

## Future Improvements

- Add persistence — either a simple JSON/XML file (matching the pattern already used in the Shape Drawing App) or a proper embedded database via Qt's SQL module (`QSqlDatabase` with SQLite).
- Close the 6–9 year highlighting gap, or make the colour scale continuous rather than three discrete bands.
- Either rename the repository/app to reflect its actual scope (e.g., "Journal Reference Manager") or genuinely broaden it to support multiple reference types (books, theses, conference papers) with type-specific fields.
- Export the table to BibTeX or CSV for use in actual academic writing workflows — this would turn the project from a demonstration into something genuinely useful.
- Split `MainWindow` into a proper model class and a thinner UI controller as the feature set grows, mirroring the cleaner separation already present in the Shape Drawing App.

## Skills Demonstrated

- Qt Model/View architecture (`QStandardItemModel`, `QSortFilterProxyModel`)
- Custom delegate development (`QStyledItemDelegate`)
- Input validation (`QRegularExpressionValidator`, regex design)
- Signal/slot–driven reactive updates
- Sorting & filtering algorithms via proxy models
- Qt Widgets GUI development
- CMake build configuration
- Git & GitHub-based project delivery

## Credits

Built with the [Qt Framework](https://www.qt.io/) (Widgets module) and [CMake](https://cmake.org/).

## License

This project is licensed under the MIT License.

```
MIT License

Copyright (c) 2026 Philasande Gumede

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
