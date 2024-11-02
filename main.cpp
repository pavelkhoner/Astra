#include <QApplication>
#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QScreen>
#include <QScroller>
#include <QTreeView>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QFileInfo>
#include <QPushButton>
#include <QMessageBox>
#include <QDir>
#include <QDebug>


// Класс для перевода размера файла/директории из байтов в более удобный формат
// на вход принимает размер типа qint64
// на выходе дает преобразованный формат КБ, МБ или ГБ
QString formatSize(qint64 size) {
    if (size < 0) {
            return "Некорректный размер"; // Если размер отрицательный
        } else if (size < 1024) {
            return QString("%1 байт").arg(size);
        } else if (size < 1024 * 1024) {
            return QString("%1 КБ").arg(size / 1024.0, 0, 'f', 2); // 2 знака после запятой
        } else if (size < 1024 * 1024 * 1024) {
            return QString("%1 МБ").arg(size / (1024.0 * 1024.0), 0, 'f', 2); // 2 знака после запятой
        } else {
            return QString("%1 ГБ").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2); // 2 знака после запятой
        }
    }



// Класс наследующийся от QFileSystemModel.
// В котором происходит фильтрация отображения для всех файлов и директорий, кроме текущей и родительской.
class CustomFileSystemModel : public QFileSystemModel {
public:
    CustomFileSystemModel(QObject *parent = nullptr) : QFileSystemModel(parent) {
        setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    }

    // Переопределение метода data для колоки "размер"
    // на вход приннимает индекс и роль данных
    // возвращает размер директории или размер файла
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole) {
            if (index.column() == 1) {
                QString path = filePath(index);
                QFileInfo fileInfo(path);

                if (fileInfo.isDir()) {
                    return formatSize(calculateDirectorySize(path)); // Размер директории
                } else {
                    return formatSize(fileInfo.size()); // Размер файла
                }
            }
        }
        return QFileSystemModel::data(index, role);
    }

    // Рекусрсивный метод вычислениия размера файлов в укзанной директории. Получая список файлов он суммирует размеры
    // На вход принимает путь к ддиректори, на выходе дает размер
    qint64 calculateDirectorySize(const QString &directoryPath) const {
        qint64 totalSize = 0;
            QDir dir(directoryPath);

            // Получаем список всех файлов в директории
            QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
            foreach (const QFileInfo &fileInfo, fileList) {
                totalSize += fileInfo.size(); // Суммируем размер файлов
            }

            // Получаем список всех поддиректорий
            QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
            foreach (const QFileInfo &dirInfo, dirList) {
                totalSize += calculateDirectorySize(dirInfo.filePath()); // Рекурсивно суммируем размер поддиректорий
            }
        return totalSize;
    }
};


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription("Qt Dir View Example");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption dontUseCustomDirectoryIconsOption("c", "Set QFileSystemModel::DontUseCustomDirectoryIcons");
    parser.addOption(dontUseCustomDirectoryIconsOption);
    QCommandLineOption dontWatchOption("w", "Set QFileSystemModel::DontWatch");
    parser.addOption(dontWatchOption);
    parser.addPositionalArgument("directory", "The directory to start in.");
    parser.process(app);
    const QString rootPath = parser.positionalArguments().isEmpty()
        ? QDir::homePath() : parser.positionalArguments().first();

    CustomFileSystemModel *model = new CustomFileSystemModel;
    model->setRootPath("");

    if (parser.isSet(dontUseCustomDirectoryIconsOption))
        model->setOption(QFileSystemModel::DontUseCustomDirectoryIcons);
    if (parser.isSet(dontWatchOption))
        model->setOption(QFileSystemModel::DontWatchForChanges);

    QTreeView *tree = new QTreeView;
    tree->setModel(model);

    // Устанавливаем корневой путь
    QModelIndex rootIndex = model->index(QDir::cleanPath(rootPath));
    if (rootIndex.isValid())
        tree->setRootIndex(rootIndex);

    // Создаем QLineEdit для фильтрации
    QLineEdit *filterEdit = new QLineEdit();
    filterEdit->setPlaceholderText("Введите имя файла или папки для фильтрации");

    // Подключаем сигнал textChanged к функции для фильтрации
    QObject::connect(filterEdit, &QLineEdit::textChanged, [model](const QString &filter) {
        if (filter.isEmpty()) {
            model->setNameFilters({});
        } else {
            model->setNameFilters({ "*" + filter + "*" });
        }
        model->setNameFilterDisables(false);
    });

    // Настраиваем виджеты
    QWidget window;
    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->addWidget(filterEdit);
    layout->addWidget(tree);

    // Настройки QTreeView
    tree->setAnimated(false);
    tree->setIndentation(20);
    tree->setSortingEnabled(true);
    const QSize availableSize = tree->screen()->availableGeometry().size();
    tree->resize(availableSize / 2);
    tree->setColumnWidth(0, tree->width() / 3);
    QScroller::grabGesture(tree, QScroller::TouchGesture);

    window.setWindowTitle(QObject::tr("Dir View"));
    window.show();

    return app.exec();
}
