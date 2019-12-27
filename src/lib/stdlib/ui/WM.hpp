#ifndef _CUBE_WM_HPP_
#define _CUBE_WM_HPP_

#include <map>
#include <string>
#include <vector>
#include <QtWidgets>

class WM
{
    private:
        QApplication *app;
        std::map<int, QWidget*> windows;
        std::vector<std::string> registeredEvents, triggeredEvents;

        bool quit;

        QWidget* getWindow(int id);
    public:
        WM(QApplication *app);
        ~WM();

        void Init();
        void Destroy();

        bool HasQuit();

        int CreateWindow(const char* title, int winWidth = 1024, int winHeight = 768);
        void DestroyWindow(int id);
        std::vector<std::string> Cycle();
        void Exec();
        bool Load(int id, const char *fileName);
        bool RegisterEvent(int id, const char *objName, const char *eventName);

        bool eventFilter(QObject *object, QEvent *event);
};


#endif