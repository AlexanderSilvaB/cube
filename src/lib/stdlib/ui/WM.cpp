#include "WM.hpp"
#include <iostream>
#include <QString>
#include <QUiLoader>
#include <QQuickWidget>
#include <QUrl>

using namespace std;

class WindowWidget : public QWidget
{
    private:
        WM *wm;
    public:
        WindowWidget(QWidget* parent = 0) : QWidget(parent)
        {
            wm = NULL;
            installEventFilter(this);
        }

        void setWM(WM *wm)
        {
            this->wm = wm;
        }

        bool eventFilter(QObject * object, QEvent * event)
        {
            if(this->wm == NULL)
                return false;
            return wm->eventFilter(object, event);
        }
};

WM::WM(QApplication *app)
{
    this->app = app;
    quit = false;
    app = NULL;
}

WM::~WM()
{

}

bool WM::HasQuit()
{
    return quit;
}

void WM::Init()
{
    
}

void WM::Destroy()
{
    
}

int WM::CreateWindow(const char* title, int winWidth, int winHeight)
{
    int id = windows.size();

    WindowWidget *window = new WindowWidget();
    window->setWM(this);
    window->resize(winWidth, winHeight);
    windows[id] = window;

    window->setProperty("cube_window_id", id);
    window->show();
    window->setWindowTitle(QApplication::translate("toplevel", title));
    return id;
}

void WM::DestroyWindow(int id)
{
    std::map<int, QWidget*>::iterator it = windows.find(id);
    if(it == windows.end())
        return;
    
    QWidget *window = it->second;
    delete window;
    windows.erase(id);

}

std::vector<std::string> WM::Cycle()
{   
    triggeredEvents.clear();

    app->processEvents();

    bool visible = false;
    std::map<int, QWidget*>::iterator it;
    for(it = windows.begin(); it != windows.end(); it++)
    {
        visible |= it->second->isVisible();
    }

    quit = !visible;

    return triggeredEvents;
}

void WM::Exec()
{
    app->exec();
}

QWidget* WM::getWindow(int id)
{
    std::map<int, QWidget*>::iterator it = windows.find(id);
    if(it == windows.end())
        return NULL;
    return it->second;
}

bool WM::Load(int id, const char *fileName)
{
    QWidget *window = getWindow(id);
    if(window == NULL)
        return false;

    QString name(fileName);

    if(name.endsWith(".ui"))
    {
        QUiLoader loader;
        //loader.setRoot(window);

        QFile file(name);
        file.open(QFile::ReadOnly);
        QWidget *myWidget = loader.load(&file, window);
        file.close();
        if(myWidget == NULL)
            return false;

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(myWidget);
        window->setLayout(layout);

        return true;
    }
    else if(name.endsWith(".qml"))
    {
        QQuickWidget *qmlView = new QQuickWidget;
        qmlView->setSource(QUrl::fromLocalFile(name));
        while(qmlView->status() == QQuickWidget::Loading)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        if(qmlView->status() == QQuickWidget::Error)
            return false;

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(qmlView);
        window->setLayout(layout);
        return true;
    }
    return false;
}


template<typename EnumType>
QString ToString(const EnumType& enumValue)
{
    const char* enumName = qt_getEnumName(enumValue);
    const QMetaObject* metaObject = qt_getEnumMetaObject(enumValue);
    if (metaObject)
    {
        const int enumIndex = metaObject->indexOfEnumerator(enumName);
        // return QString("%1::%2::%3").arg(metaObject->className(), enumName, metaObject->enumerator(enumIndex).valueToKey(enumValue));
        return QString("%1").arg(metaObject->enumerator(enumIndex).valueToKey(enumValue));
    }

    //return QString("%1::%2").arg(enumName).arg(static_cast<int>(enumValue));
    return QString("%1").arg(static_cast<int>(enumValue));
}

bool WM::eventFilter(QObject *object, QEvent *event)
{
    QString qstr = ToString(event->type());
    QVariant idQ = object->property("cube_window_id");
    string id = "";
    if(idQ.isValid())
    {
        id = QString::number(idQ.toInt()).toStdString();
    }
    string onId = id + ":" + object->objectName().toStdString() + ":" + qstr.toStdString();
    vector<string>::iterator it = find(registeredEvents.begin(), registeredEvents.end(), onId);
    if(it != registeredEvents.end())
    {
        triggeredEvents.push_back(*it);
    }
    return false;
}

bool WM::RegisterEvent(int id, const char *objName, const char *eventName)
{
    QWidget *window = getWindow(id);
    if(window == NULL)
        return false;

    string sid = QString::number(id).toStdString();

    string onId = sid + ":" + string(objName) + ":" + string(eventName);
    registeredEvents.push_back(onId);

    QList<QWidget *> widgets = window->findChildren<QWidget *>(objName);
    for( int i=0; i<widgets.count(); ++i )
    {
        widgets[i]->installEventFilter(window);
        widgets[i]->setProperty("cube_window_id", id);
    }
}