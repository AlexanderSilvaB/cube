/**
 * @Author: Alexander Silva Barbosa
 * @Date:   1969-12-31 21:00:00
 * @Last Modified by:   Alexander Silva Barbosa
 * @Last Modified time: 2021-09-25 00:24:11
 */
#include "WM.hpp"
#include "Shapes.hpp"
#include <QQuickWidget>
#include <QString>
#include <QUiLoader>
#include <QUrl>
#include <cstring>
#include <iostream>

using namespace std;

class WindowWidget : public QWidget
{
  private:
    int ids;
    WM *wm;
    QVector<Shape *> shapes;

  public:
    bool antialias;

    WindowWidget(QWidget *parent = 0) : QWidget(parent)
    {
        antialias = true;
        ids = 0;
        wm = NULL;
        installEventFilter(this);
    }

    ~WindowWidget()
    {
        for (int i = 0; i < shapes.size(); i++)
        {
            delete shapes[i];
        }
        shapes.clear();
    }

    int addShape(Shape *shape)
    {
        shape->id = ids++;
        shapes.append(shape);
        return shape->id;
    }

    Shape *getShape(int id)
    {
        int i = -1;
        for (i = 0; i < shapes.size(); i++)
        {
            if (shapes[i]->id == id)
            {
                break;
            }
        }

        if (i >= 0 && i < shapes.size())
        {
            return shapes[i];
        }

        return NULL;
    }

    void rmShape(int id)
    {
        int i = -1;
        for (i = 0; i < shapes.size(); i++)
        {
            if (shapes[i]->id == id)
            {
                break;
            }
        }

        if (i >= 0 && i < shapes.size())
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
        if (layout() == nullptr)
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, antialias);
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
    for (map<int, QWidget *>::iterator it = windows.begin(); it != windows.end(); it++)
    {
        QWidget *window = it->second;
        delete window;
    }
    windows.clear();
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
        // loader.setRoot(window);

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

template <typename EnumType> QString ToString(const EnumType &enumValue)
{
    const char *enumName = qt_getEnumName(enumValue);
    const QMetaObject *metaObject = qt_getEnumMetaObject(enumValue);
    if (metaObject)
    {
        const int enumIndex = metaObject->indexOfEnumerator(enumName);
        // return QString("%1::%2::%3").arg(metaObject->className(), enumName,
        // metaObject->enumerator(enumIndex).valueToKey(enumValue));
        return QString("%1").arg(metaObject->enumerator(enumIndex).valueToKey(enumValue));
    }

    // return QString("%1::%2").arg(enumName).arg(static_cast<int>(enumValue));
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

    if (string(objName) == sid)
    {
        window->installEventFilter(window);
        window->setProperty("cube_window_id", id);
        window->setProperty("cube_event_name", true);
        return true;
    }

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
    return true;
}

Dict WM::createEventDict(QObject *obj, QEvent *event)
{
    Dict dict;
    switch (event->type())
    {
        case QEvent::Enter: {
            QEnterEvent *ev = static_cast<QEnterEvent *>(event);
            dict["x"] = QString::number(ev->x()).toStdString();
            dict["y"] = QString::number(ev->y()).toStdString();
            break;
        }
        case QEvent::FocusIn:
        case QEvent::FocusOut: {
            QFocusEvent *ev = static_cast<QFocusEvent *>(event);
            dict["got"] = ev->gotFocus() ? "true" : "false";
            dict["lost"] = ev->lostFocus() ? "true" : "false";
            break;
        }
        case QEvent::Move: {
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
        case QEvent::MouseMove: {
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
        case QEvent::KeyRelease: {
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
        for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
            v.emplace_back(mo->property(i).name(), mo->property(i).read(o));
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
            if (strcmp(name, mo->property(i).name()) == 0)
            {
                return mo->property(i).read(o);
            }
        }
    } while ((mo = mo->superClass()));
    return QVariant();
}

static bool set_prop(QObject *o, const char *name, const char *value)
{
    QVariant val(value);
    auto mo = o->metaObject();
    do
    {
        for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
        {
            if (strcmp(name, mo->property(i).name()) == 0)
            {
                if (mo->property(i).isWritable())
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

static void get_props(QObject *o, Dict &dict)
{
    auto mo = o->metaObject();
    do
    {
        for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i)
        {
            if (mo->property(i).isReadable())
                dict[string(mo->property(i).name())] = mo->property(i).read(o).toString().toStdString();
        }
    } while ((mo = mo->superClass()));
}

static void get_methods(QObject *o, List &list)
{
    auto mo = o->metaObject();
    do
    {
        for (int i = mo->methodOffset(); i < mo->methodCount(); ++i)
        {
            list.push_back(QString::fromLatin1(mo->method(i).methodSignature()).toStdString());
        }
    } while ((mo = mo->superClass()));
}

static string get_method(QObject *o, const char *name)
{
    auto mo = o->metaObject();
    string m_name;

    do
    {
        for (int i = mo->methodOffset(); i < mo->methodCount(); ++i)
        {
            m_name = QString::fromLatin1(mo->method(i).name()).toStdString();
            if (strcmp(name, m_name.c_str()) == 0)
            {
                m_name = QString::fromLatin1(mo->method(i).methodSignature()).toStdString();
                return m_name;
            }
        }
    } while ((mo = mo->superClass()));
    return "";
}

static bool call(QObject *object, QMetaMethod metaMethod, QVariantList &args, QVariant &ret)
{
    // Convert the arguments

    QVariantList converted;

    // We need enough arguments to perform the conversion.

    QList<QByteArray> methodTypes = metaMethod.parameterTypes();
    if (methodTypes.size() < args.size())
    {
        // qWarning() << "Insufficient arguments to call" << metaMethod.methodSignature();
        return false;
    }

    for (int i = 0; i < methodTypes.size() && i < args.size(); i++)
    {
        const QVariant &arg = args.at(i);

        QByteArray methodTypeName = methodTypes.at(i);
        QByteArray argTypeName = arg.typeName();

        QVariant::Type methodType = QVariant::nameToType(methodTypeName);
        QVariant::Type argType = arg.type();

        QVariant copy = QVariant(arg);

        // If the types are not the same, attempt a conversion. If it
        // fails, we cannot proceed.

        if (copy.type() != methodType)
        {
            if (copy.canConvert(methodType))
            {
                if (!copy.convert(methodType))
                {
                    // qWarning() << "Cannot convert" << argTypeName << "to" << methodTypeName;
                    return false;
                }
            }
        }

        converted << copy;
    }

    QList<QGenericArgument> arguments;

    for (int i = 0; i < converted.size(); i++)
    {

        // Notice that we have to take a reference to the argument, else
        // we'd be pointing to a copy that will be destroyed when this
        // loop exits.

        QVariant &argument = converted[i];

        // A const_cast is needed because calling data() would detach
        // the QVariant.

        QGenericArgument genericArgument(QMetaType::typeName(argument.userType()),
                                         const_cast<void *>(argument.constData()));

        arguments << genericArgument;
    }

    QVariant returnValue(QMetaType::type(metaMethod.typeName()), static_cast<void *>(NULL));

    QGenericReturnArgument returnArgument(metaMethod.typeName(), const_cast<void *>(returnValue.constData()));

    // Perform the call

    bool ok = metaMethod.invoke(object, Qt::DirectConnection, returnArgument, arguments.value(0), arguments.value(1),
                                arguments.value(2), arguments.value(3), arguments.value(4), arguments.value(5),
                                arguments.value(6), arguments.value(7), arguments.value(8), arguments.value(9));

    if (!ok)
    {
        // qWarning() << "Calling" << metaMethod.methodSignature() << "failed.";
        return false;
    }
    else
    {
        ret = returnValue;
        return true;
    }
}

static bool call_method(QObject *o, const char *name, const List &args, List &ret)
{
    auto mo = o->metaObject();
    string m_name;

    bool success = false;

    QVariantList list;
    for (int i = 0; i < args.size(); i++)
    {
        list << QVariant(args[i].c_str());
    }

    do
    {
        for (int i = mo->methodOffset(); i < mo->methodCount(); ++i)
        {
            m_name = QString::fromLatin1(mo->method(i).name()).toStdString();
            if (strcmp(name, m_name.c_str()) == 0)
            {
                QVariant r;
                if (call(o, mo->method(i), list, r))
                {
                    success |= true;
                    if (r.isValid())
                    {
                        ret.push_back(r.toString().toStdString());
                    }
                    else
                    {
                        ret.push_back("");
                    }
                }
            }
            else
            {
                m_name = QString::fromLatin1(mo->method(i).methodSignature()).toStdString();
                if (strcmp(name, m_name.c_str()) == 0)
                {
                    QVariant r;
                    if (call(o, mo->method(i), list, r))
                    {
                        success |= true;
                        if (r.isValid())
                        {
                            ret.push_back(r.toString().toStdString());
                        }
                        else
                        {
                            ret.push_back("");
                        }
                    }
                }
            }
        }
    } while ((mo = mo->superClass()));
    return success;
}

List WM::GetProperty(int id, const char *objName, const char *propName)
{
    List list;

    QWidget *window = getWindow(id);
    if (window == NULL)
        return list;

    if (objName == NULL || objName[0] == '\0')
    {
        QVariant val = get_prop(window, propName);
        if (val.isValid())
        {
            list.push_back(val.toString().toStdString());
        }
        return list;
    }

    QList<QWidget *> widgets = window->findChildren<QWidget *>(objName);

    if (widgets.count() > 0)
    {
        for (int i = 0; i < widgets.count(); ++i)
        {
            QVariant val = get_prop(widgets[i], propName);
            if (val.isValid())
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
                if (val.isValid())
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

    if (objName == NULL || objName[0] == '\0')
    {
        return set_prop(window, propName, value);
    }

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

    if (objName == NULL || objName[0] == '\0')
    {
        get_props(window, dict);
        return dict;
    }

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

List WM::GetMethods(int id, const char *objName)
{
    List list;

    QWidget *window = getWindow(id);
    if (window == NULL)
        return list;

    if (objName == NULL || objName[0] == '\0')
    {
        get_methods(window, list);
        return list;
    }

    QList<QWidget *> widgets = window->findChildren<QWidget *>(objName);

    if (widgets.count() > 0)
    {
        if (widgets.count() >= 1)
        {
            get_methods(widgets[0], list);
        }
    }
    else
    {
        widgets = window->findChildren<QWidget *>();
        for (int i = 0; i < widgets.count(); ++i)
        {
            if (widgets[i]->inherits(objName))
            {
                get_methods(widgets[0], list);
                break;
            }
        }
    }

    return list;
}

std::string WM::GetMethod(int id, const char *objName, const char *methodName)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return "";

    if (objName == NULL || objName[0] == '\0')
    {
        return get_method(window, methodName);
    }

    QList<QWidget *> widgets = window->findChildren<QWidget *>(objName);

    if (widgets.count() > 0)
    {
        if (widgets.count() >= 1)
        {
            return get_method(widgets[0], methodName);
        }
    }
    else
    {
        widgets = window->findChildren<QWidget *>();
        for (int i = 0; i < widgets.count(); ++i)
        {
            if (widgets[i]->inherits(objName))
            {
                return get_method(widgets[0], methodName);
            }
        }
    }
    return "";
}

bool WM::CallMethod(int id, const char *objName, const char *methodName, const List &arguments, List &ret)
{
    bool success = false;

    QWidget *window = getWindow(id);
    if (window == NULL)
        return success;

    if (objName == NULL || objName[0] == '\0')
    {
        return call_method(window, methodName, arguments, ret);
    }

    QList<QWidget *> widgets = window->findChildren<QWidget *>(objName);

    if (widgets.count() > 0)
    {
        for (int i = 0; i < widgets.count(); ++i)
        {
            success |= call_method(widgets[i], methodName, arguments, ret);
        }
    }
    else
    {
        widgets = window->findChildren<QWidget *>();
        for (int i = 0; i < widgets.count(); ++i)
        {
            if (widgets[i]->inherits(objName))
            {
                success |= call_method(widgets[i], methodName, arguments, ret);
            }
        }
    }

    return success;
}

void WM::Resize(int id, int winWidth, int winHeight)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return;

    window->resize(winWidth, winHeight);
}

// Shapes

void WM::SetAntialias(int id, bool antialias)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return;

    ((WindowWidget *)window)->antialias = antialias;
}

int WM::AddShape(int id, Dict &values)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return -1;

    if (values.find("type") == values.end())
        return -1;

    string type = values["type"];
    Shape *shape = NULL;
    if (type == "rect")
        shape = new RectShape();
    else if (type == "arc")
        shape = new ArcShape();
    else if (type == "text")
        shape = new TextShape();
    else if (type == "point")
        shape = new PointShape();
    else if (type == "line")
        shape = new LineShape();
    else if (type == "ellipse")
        shape = new EllipseShape();
    else if (type == "circle")
        shape = new CircleShape();
    else if (type == "image")
        shape = new ImageShape();
    else
        return -1;

    shape->update(values);
    window->repaint();
    return ((WindowWidget *)window)->addShape(shape);
}

void WM::UpdateShape(int id, Dict &values)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return;

    if (values.find("id") == values.end())
        return;

    int shapeId = atoi(values["id"].c_str());

    Shape *shape = ((WindowWidget *)window)->getShape(shapeId);
    if (shape != NULL)
    {
        shape->update(values);
        window->repaint();
    }
}

void WM::RmShape(int id, int shape)
{
    QWidget *window = getWindow(id);
    if (window == NULL)
        return;

    ((WindowWidget *)window)->rmShape(shape);
}