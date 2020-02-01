#ifndef _CUBE_WM_HPP_
#define _CUBE_WM_HPP_

#include <map>
#include <string>
#include <vector>
#include <QtWidgets>

typedef std::map<std::string, std::string> Dict;
typedef std::vector<std::string> List;

class WM
{
    private:
        QApplication *app;
        std::map<int, QWidget*> windows;
        List registeredEvents, triggeredEvents;
        std::map<std::string, Dict> triggeredEventsArgs;

        bool quit;

        QWidget* getWindow(int id);
        Dict createEventDict(QObject *obj, QEvent *event);
    public:
        WM(QApplication *app);
        ~WM();

        void Init();
        void Destroy();

        bool HasQuit();

        int NewWindow(const char* title, int winWidth = 1024, int winHeight = 768);
        void DestroyWindow(int id);
        List Cycle();
        void Exec();
        bool Load(int id, const char *fileName);
        bool RegisterEvent(int id, const char *objName, const char *eventName);
        Dict GetEventArgs(const char* name);
        List GetProperty(int id, const char *objName, const char *propName);
        bool SetProperty(int id, const char *objName, const char *propName, const char *value);
        Dict GetProperties(int id, const char *objName);


        bool eventFilter(QObject *object, QEvent *event);
};


#endif