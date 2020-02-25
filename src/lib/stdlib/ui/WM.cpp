#include "WM.hpp"
#include <iostream>
#include <cstring>
#include <QString>
#include <QUiLoader>
#include <QQuickWidget>
#include <QUrl>
#include "Shapes.hpp"

using namespace std;

class WindowWidget : public QWidget
{
private:
    int ids;
    WM *wm;
    QVector<Shape*> shapes;

public:
    WindowWidget(QWidget *parent = 0) : QWidget(parent)
    {
        ids = 0;
        wm = NULL;
        installEventFilter(this);
    }

    int addShape(Shape *shape)
    {
        shape->id = ids++;
        shapes.append(shape);
        return shape->id;
    }

    Shape* getShape(int id)
    {
        int i = -1;
        for(i = 0; i < shapes.size(); i++)
        {
            if(shapes[i]->id == id)
            {
                break;
            }
        }

        if(i >= 0 && i < shapes.size())
        {
            return shapes[i];
        }

        return NULL;
    }

    void rmShape(int id)
    {
        int i = -1;
        for(i = 0; i < shapes.size(); i++)
        {
            if(shapes[i]->id == id)
            {
                break;
            }
        }

        if(i >= 0 && i < shapes.size())
        {
            Shape *s = shapes[i];
            shapes.remove(i);
            delete s;
        }
    }

    void setWM(WM *wm)
    {
        this->wm = wm;
    }

    bool eventFilter(QObject *object, QEvent *event)
    {
        if (this->wm == NULL)
            return false;
        return wm->eventFilter(object, event);
    }

    void paintEvent(QPaintEvent *event)
    {
        if(layout() == nullptr)
        {
            QPainter painter(this);
            for (int i = 0; i < shapes.size(); ++i) 
            {
                shapes[i]->set(painter);
                shapes[i]->draw(painter);
            }
        }
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

int WM::NewWindow(const char *title, int winWidth, int winHeight)
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
    std::map<int, QWidget *>::iterator it = windows.find(id);
    if (it == windows.end())
        return;

    QWidget *window = it->second;
    delete window;
    windows.erase(id);
}

List WM::Cycle()
{
    triggeredEvents.clear();
    triggeredEventsArgs.clear();

    app->processEvents();

    bool visible = false;
    std::map<int, QWidget *>::iterator it;
    for (it = windows.begin(); it != windows.end(); it++)
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

QWidget *WM::getWindow(int id)
{
    std::map<int, QWidget *>::iterator it = windows.find(id);
    if (it == windows.end())
        return NULL;
    return it->second;
}

bool WM::Load(int id, const char *fileName)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return false;

    QString name(fileName);

    if (name.endsWith(".ui"))
    {
        QUiLoader loader;
        //loader.setRoot(window);

        QFile file(name);
        file.open(QFile::ReadOnly);
        QWidget *myWidget = loader.load(&file, window);
        file.close();
        if (myWidget == NULL)
            return false;

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(myWidget);
        window->setLayout(layout);

        return true;
    }
    else if (name.endsWith(".qml"))
    {
        QQuickWidget *qmlView = new QQuickWidget;
        qmlView->setSource(QUrl::fromLocalFile(name));
        while (qmlView->status() == QQuickWidget::Loading)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        if (qmlView->status() == QQuickWidget::Error)
            return false;

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(qmlView);
        window->setLayout(layout);
        return true;
    }
    return false;
}

template <typename EnumType>
QString ToString(const EnumType &enumValue)
{
    const char *enumName = qt_getEnumName(enumValue);
    const QMetaObject *metaObject = qt_getEnumMetaObject(enumValue);
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
    QVariant useNameQ = object->property("cube_event_name");
    string id = "";
    bool useName = true;
    if (idQ.isValid())
    {
        id = QString::number(idQ.toInt()).toStdString();
    }
    if (useNameQ.isValid())
    {
        useName = useNameQ.toBool();
    }
    string onId;
    if (useName)
        onId = id + ":" + object->objectName().toStdString() + ":" + qstr.toStdString();
    else
        onId = id + ":" + object->metaObject()->className() + ":" + qstr.toStdString();
    List::iterator it = find(registeredEvents.begin(), registeredEvents.end(), onId);
    if (it != registeredEvents.end())
    {
        onId += ":" + object->objectName().toStdString();
        triggeredEvents.push_back(onId);
        triggeredEventsArgs[onId] = createEventDict(object, event);
    }
    return false;
}

bool WM::RegisterEvent(int id, const char *objName, const char *eventName)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return false;

    string sid = QString::number(id).toStdString();

    string onId = sid + ":" + string(objName) + ":" + string(eventName);
    registeredEvents.push_back(onId);

    QList<QWidget *> widgets = window->findChildren<QWidget *>(objName);
    if (widgets.count() > 0)
    {
        for (int i = 0; i < widgets.count(); ++i)
        {
            widgets[i]->installEventFilter(window);
            widgets[i]->setProperty("cube_window_id", id);
            widgets[i]->setProperty("cube_event_name", true);
        }
    }
    else
    {
        widgets = window->findChildren<QWidget *>();
        for (int i = 0; i < widgets.count(); ++i)
        {
            if (widgets[i]->inherits(objName))
            {
                widgets[i]->installEventFilter(window);
                widgets[i]->setProperty("cube_window_id", id);
                widgets[i]->setProperty("cube_event_name", false);
            }
        }
    }
}

Dict WM::createEventDict(QObject *obj, QEvent *event)
{
    Dict dict;
    switch (event->type())
    {
    case QEvent::Enter:
    {
        QEnterEvent *ev = static_cast<QEnterEvent *>(event);
        dict["x"] = QString::number(ev->x()).toStdString();
        dict["y"] = QString::number(ev->y()).toStdString();
        break;
    }
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    {
        QFocusEvent *ev = static_cast<QFocusEvent *>(event);
        dict["got"] = ev->gotFocus() ? "true" : "false";
        dict["lost"] = ev->lostFocus() ? "true" : "false";
        break;
    }
    case QEvent::Move:
    {
        QMoveEvent *ev = static_cast<QMoveEvent *>(event);
        dict["x"] = QString::number(ev->pos().x()).toStdString();
        dict["y"] = QString::number(ev->pos().y()).toStdString();
        dict["oldx"] = QString::number(ev->oldPos().x()).toStdString();
        dict["oldy"] = QString::number(ev->oldPos().y()).toStdString();
        break;
    }
    case QEvent::NonClientAreaMouseButtonDblClick:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseMove:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    {
        QMouseEvent *ev = static_cast<QMouseEvent *>(event);
        dict["x"] = QString::number(ev->x()).toStdString();
        dict["y"] = QString::number(ev->y()).toStdString();
        dict["globalx"] = QString::number(ev->globalX()).toStdString();
        dict["globaly"] = QString::number(ev->globalY()).toStdString();

        Qt::MouseButtons mouse = ev->buttons();
        dict["left"] = (mouse & Qt::LeftButton) != 0 ? "true" : "false";
        dict["right"] = (mouse & Qt::RightButton) != 0 ? "true" : "false";
        dict["mid"] = (mouse & Qt::MidButton) != 0 ? "true" : "false";
        dict["back"] = (mouse & Qt::BackButton) != 0 ? "true" : "false";
        dict["forward"] = (mouse & Qt::ForwardButton) != 0 ? "true" : "false";
        dict["task"] = (mouse & Qt::TaskButton) != 0 ? "true" : "false";

        break;
    }
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
        QKeyEvent *ev = static_cast<QKeyEvent *>(event);
        dict["count"] = QString::number(ev->count()).toStdString();
        dict["key"] = QString::number(ev->key()).toStdString();
        dict["modifiers"] = QString::number(ev->nativeModifiers()).toStdString();
        break;
    }
    }

    return dict;
}

Dict WM::GetEventArgs(const char *name)
{
    string strName(name);
    if (triggeredEventsArgs.find(strName) == triggeredEventsArgs.end())
    {
        return Dict();
    }

    return triggeredEventsArgs[strName];
}

static void dump_props(QObject *o)
{
    auto mo = o->metaObject();
    qDebug() << "## Properties of" << o << "##";
    do
    {
        qDebug() << "### Class" << mo->className() << "###";
        std::vector<std::pair<QString, QVariant>> v;
        v.reserve(mo->propertyCount() - mo->propertyOffset());
        for (int i = mo->propertyOffset(); i < mo->propertyCount();
             ++i)
            v.emplace_back(mo->property(i).name(),
                           mo->property(i).read(o));
        std::sort(v.begin(), v.end());
        for (auto &i : v)
            qDebug() << i.first << "=>" << i.second;
    } while ((mo = mo->superClass()));
}

static QVariant get_prop(QObject *o, const char *name)
{
    auto mo = o->metaObject();
    do
    {
        for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
        {
            if(strcmp(name, mo->property(i).name()) == 0)
            {
                return mo->property(i).read(o);
            }
        }
    } while ((mo = mo->superClass()));
    return QVariant();
}

static bool set_prop(QObject *o, const char *name, const char* value)
{
    QVariant val(value);
    auto mo = o->metaObject();
    do
    {
        for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
        {
            if(strcmp(name, mo->property(i).name()) == 0)
            {
                if(mo->property(i).isWritable())
                {
                    return mo->property(i).write(o, val);
                }
                else
                    return false;
            }
        }
    } while ((mo = mo->superClass()));
    return false;
}

static void get_props(QObject *o, Dict& dict)
{
    auto mo = o->metaObject();
    do
    {
        for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
        {
            if(mo->property(i).isReadable())
                dict[string(mo->property(i).name())] = mo->property(i).read(o).toString().toStdString();
        }
    } while ((mo = mo->superClass()));
}

List WM::GetProperty(int id, const char *objName, const char *propName)
{
    List list;

    QWidget *window = getWindow(id);
    if (window == NULL)
        return list;

    QList<QWidget *> widgets = window->findChildren<QWidget *>(objName);

    if (widgets.count() > 0)
    {
        for (int i = 0; i < widgets.count(); ++i)
        {
            QVariant val = get_prop(widgets[i], propName);
            if(val.isValid())
            {
                list.push_back(val.toString().toStdString());
            }
        }
    }
    else
    {
        widgets = window->findChildren<QWidget *>();
        for (int i = 0; i < widgets.count(); ++i)
        {
            if (widgets[i]->inherits(objName))
            {
                QVariant val = get_prop(widgets[i], propName);
                if(val.isValid())
                {
                    list.push_back(val.toString().toStdString());
                }
            }
        }
    }

    return list;
}

bool WM::SetProperty(int id, const char *objName, const char *propName, const char *value)
{
    bool success = false;

    QWidget *window = getWindow(id);
    if (window == NULL)
        return success;

    QList<QWidget *> widgets = window->findChildren<QWidget *>(objName);

    if (widgets.count() > 0)
    {
        for (int i = 0; i < widgets.count(); ++i)
        {
            success |= set_prop(widgets[i], propName, value);
        }
    }
    else
    {
        widgets = window->findChildren<QWidget *>();
        for (int i = 0; i < widgets.count(); ++i)
        {
            if (widgets[i]->inherits(objName))
            {
                success |= set_prop(widgets[i], propName, value);
            }
        }
    }

    return success;
}

Dict WM::GetProperties(int id, const char *objName)
{
    Dict dict;

    QWidget *window = getWindow(id);
    if (window == NULL)
        return dict;

    QList<QWidget *> widgets = window->findChildren<QWidget *>(objName);

    if (widgets.count() > 0)
    {
        if (widgets.count() >= 1)
        {
            get_props(widgets[0], dict);
        }
    }
    else
    {
        widgets = window->findChildren<QWidget *>();
        for (int i = 0; i < widgets.count(); ++i)
        {
            if (widgets[i]->inherits(objName))
            {
                get_props(widgets[0], dict);
                break;
            }
        }
    }

    return dict;
}

// Shapes

int WM::AddShape(int id, Dict &values)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return -1;
    
    if(values.find("type") == values.end())
        return -1;

    string type = values["type"];
    Shape *shape = NULL;
    if(type == "rect")
        shape = new Rect();
    else if(type == "arc")
        shape = new Arc();
    else
        return -1;
    
    shape->update(values);
    return ((WindowWidget*)window)->addShape(shape);
}

void WM::UpdateShape(int id, Dict &values)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return;

    if(values.find("id") == values.end())
        return;

    int shapeId = atoi(values["id"].c_str());

    Shape *shape = ((WindowWidget*)window)->getShape(shapeId);
    if(shape != NULL)
        shape->update(values);
}

void WM::RmShape(int id, int shape)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return;

    ((WindowWidget*)window)->rmShape(shape);
}